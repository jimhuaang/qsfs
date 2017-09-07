// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | You may not use this work except in compliance with the License.
// | You may obtain a copy of the License in the LICENSE file, or at:
// |
// | http://www.apache.org/licenses/LICENSE-2.0
// |
// | Unless required by applicable law or agreed to in writing, software
// | distributed under the License is distributed on an "AS IS" BASIS,
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// | See the License for the specific language governing permissions and
// | limitations under the License.
// +-------------------------------------------------------------------------

#include "client/QSTransferManager.h"

#include <assert.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "client/Client.h"
#include "client/ClientConfiguration.h"
#include "client/ClientError.h"
#include "client/QSError.h"
#include "client/TransferHandle.h"
#include "client/Utils.h"
#include "data/Directory.h"
#include "data/IOStream.h"
#include "data/StreamBuf.h"
#include "filesystem/Configure.h"
#include "filesystem/Drive.h"

namespace QS {

namespace Client {

using QS::Client::Utils::BuildRequestRange;
using QS::Data::Buffer;
using QS::Data::IOStream;
using QS::Data::StreamBuf;
using QS::FileSystem::Configure::GetUploadMultipartMinPartSize;
using QS::FileSystem::Configure::GetUploadMultipartThresholdSize;
using std::iostream;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::vector;

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::DownloadFile(
    const string &filePath, off_t offset, uint64_t size,
    shared_ptr<iostream> bufStream) {
  // Drive::ReadFile has checked the object existence, so no check here.
  // Drive::ReadFile has already ajust the download size, so no ajust here.
  if (!bufStream) {
    DebugError("Null buffer stream parameter");
    return nullptr;
  }

  string bucket = ClientConfiguration::Instance().GetBucket();
  auto handle = std::make_shared<TransferHandle>(bucket, filePath, offset, size,
                                                 TransferDirection::Download);
  handle->SetDownloadStream(bufStream);

  DoDownload(handle);
  return handle;
}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::RetryDownload(
    const shared_ptr<TransferHandle> &handle, shared_ptr<iostream> bufStream) {
  if (handle->GetStatus() == TransferStatus::InProgress ||
      handle->GetStatus() == TransferStatus::Completed ||
      handle->GetStatus() == TransferStatus::NotStarted) {
    DebugWarning("Input handle is not avaialbe to retry");
    return handle;
  }

  if (handle->GetStatus() == TransferStatus::Aborted) {
    return DownloadFile(handle->GetObjectKey(), handle->GetContentRangeBegin(),
                        handle->GetBytesTotalSize(), bufStream);
  } else {
    handle->UpdateStatus(TransferStatus::NotStarted);
    handle->Restart();
    DoDownload(handle);
    return handle;
  }
}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::UploadFile(const string &filePath,
                                                         uint64_t fileSize) {
  string bucket = ClientConfiguration::Instance().GetBucket();
  auto handle = std::make_shared<TransferHandle>(bucket, filePath, 0, fileSize,
                                                 TransferDirection::Upload);
  DoUpload(handle);

  return handle;
}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::RetryUpload(
    const shared_ptr<TransferHandle> &handle) {
  if (handle->GetStatus() == TransferStatus::InProgress ||
      handle->GetStatus() == TransferStatus::Completed ||
      handle->GetStatus() == TransferStatus::NotStarted) {
    DebugWarning("Input handle is not avaialbe to retry");
    return handle;
  }

  if (handle->GetStatus() == TransferStatus::Aborted) {
    return UploadFile(handle->GetObjectKey(), handle->GetBytesTotalSize());
  } else {
    handle->UpdateStatus(TransferStatus::NotStarted);
    handle->Restart();
    DoUpload(handle);
    return handle;
  }
}

// --------------------------------------------------------------------------
void QSTransferManager::AbortMultipartUpload(
    const shared_ptr<TransferHandle> &handle) {
  if (!handle->IsMultipart()) {
    DebugWarning("Unable to abort a non multipart upload");
    return;
  }

  handle->Cancle();
  handle->WaitUntilFinished();
  if (handle->GetStatus() == TransferStatus::Cancelled) {
    auto err = GetClient()->AbortMultipartUpload(handle->GetObjectKey(),
                                                 handle->GetMultiPartId());
    if (IsGoodQSError(err)) {
      handle->UpdateStatus(TransferStatus::Aborted);
    } else {
      handle->SetError(err);
      DebugError(GetMessageForQSError(err));
    }
  }
}

// --------------------------------------------------------------------------
bool QSTransferManager::PrepareDownload(
    const shared_ptr<TransferHandle> &handle) {
  assert(GetBufferSize() > 0);
  if (!(GetBufferSize() > 0)) {
    DebugError("Buffer size is less than 0");
    return false;
  }

  bool isRetry = handle->HasParts();
  if (isRetry) {
    for (auto part : handle->GetFailedParts()) {
      handle->AddQueuePart(part.second);
    }
  } else {
    // prepare part and add it into queue
    auto totalTransferSize = handle->GetBytesTotalSize();
    size_t partCount = std::ceil(totalTransferSize / GetBufferSize());
    handle->SetIsMultiPart(partCount > 1);
    for (size_t i = 1; i < partCount; ++i) {
      // part id, best progress in bytes, part size, range begin
      handle->AddQueuePart(make_shared<Part>(
          i, 0, GetBufferSize(),
          handle->GetContentRangeBegin() + (i - 1) * GetBufferSize()));
    }
    size_t sz = totalTransferSize - (partCount - 1) * GetBufferSize();
    handle->AddQueuePart(make_shared<Part>(
        partCount, 0, std::min(sz, GetBufferSize()),
        handle->GetContentRangeBegin() + (partCount - 1) * GetBufferSize()));
  }
  return true;
}

// --------------------------------------------------------------------------
void QSTransferManager::DoSinglePartDownload(
    const shared_ptr<TransferHandle> &handle) {
  auto queuedParts = handle->GetQueuedParts();
  assert(queuedParts.size() == 1);

  const auto &part = queuedParts.begin()->second;
  handle->AddPendingPart(part);
  auto receivedHandler =
      [this, handle, part](const pair<ClientError<QSError>, string> &outcome) {
        auto &err = outcome.first;
        auto &eTag = outcome.second;
        if (IsGoodQSError(err)) {
          handle->ChangePartToCompleted(part, eTag);
          handle->UpdateStatus(TransferStatus::Completed);
        } else {
          handle->ChangePartToFailed(part);
          handle->UpdateStatus(TransferStatus::Failed);
          handle->SetError(err);
          DebugError(GetMessageForQSError(err));
        }
      };

  GetClient()->GetExecutor()->SubmitAsyncPrioritized(
      receivedHandler,
      [this, handle, part]() -> pair<ClientError<QSError>, string> {
        string eTag;
        auto err = GetClient()->DownloadFile(
            handle->GetObjectKey(),
            BuildRequestRange(part->GetRangeBegin(), part->GetSize()),
            handle->GetDownloadStream(), &eTag);
        return {err, eTag};
      });
}

// --------------------------------------------------------------------------
void QSTransferManager::DoMultiPartDownload(
    const shared_ptr<TransferHandle> &handle) {
  auto queuedParts = handle->GetQueuedParts();
  auto ipart = queuedParts.begin();

  for (; ipart != queuedParts.end() && handle->ShouldContinue(); ++ipart) {
    auto buffer = GetBufferManager()->Acquire();
    if (!buffer) {
      DebugWarning("Unable to acquire resource, stop download");
      break;
    }
    const auto &part = ipart->second;
    if (handle->ShouldContinue()) {
      part->SetDownloadPartStream(
          make_shared<IOStream>(std::move(buffer), GetBufferSize()));
      handle->AddPendingPart(part);

      auto receivedHandler = [this, handle, part](
          const pair<ClientError<QSError>, string> &outcome) {
        auto &err = outcome.first;
        auto &eTag = outcome.second;
        // write part stream to download stream
        if (IsGoodQSError(err)) {
          if (handle->ShouldContinue()) {
            handle->WritePartToDownloadStream(part->GetDownloadPartStream(),
                                              part->GetRangeBegin());
            handle->ChangePartToCompleted(part, eTag);
          } else {
            handle->ChangePartToFailed(part);
          }
        } else {
          handle->ChangePartToFailed(part);
          handle->SetError(err);
          DebugError(GetMessageForQSError(err));
        }

        // release part buffer back to resource manager
        if (part->GetDownloadPartStream()) {
          auto partStreamBuf =
              dynamic_cast<StreamBuf *>(part->GetDownloadPartStream()->rdbuf());
          if (partStreamBuf) {
            GetBufferManager()->Release(partStreamBuf->ReleaseBuffer());
            part->SetDownloadPartStream(shared_ptr<iostream>(nullptr));
          }
        }

        // update status
        if (!handle->HasPendingParts() && !handle->HasQueuedParts()) {
          if (!handle->HasFailedParts() && handle->DoneTransfer()) {
            handle->UpdateStatus(TransferStatus::Completed);
          } else {
            handle->UpdateStatus(TransferStatus::Failed);
          }
        }
      };

      string objKey = handle->GetObjectKey();
      GetClient()->GetExecutor()->SubmitAsync(
          receivedHandler,
          [this, objKey, part]() -> pair<ClientError<QSError>, string> {
            string eTag;
            auto err = GetClient()->DownloadFile(
                objKey,
                BuildRequestRange(part->GetRangeBegin(), part->GetSize()),
                part->GetDownloadPartStream(), &eTag);
            return {err, eTag};
          });
    } else {
      GetBufferManager()->Release(std::move(buffer));
      break;
    }
  }

  for (; ipart != queuedParts.end(); ++ipart) {
    handle->ChangePartToFailed(ipart->second);
  }
}

// --------------------------------------------------------------------------
void QSTransferManager::DoDownload(const shared_ptr<TransferHandle> &handle) {
  handle->UpdateStatus(TransferStatus::InProgress);
  if (!PrepareDownload(handle)) {
    return;
  }
  if (handle->IsMultipart()) {
    DoMultiPartDownload(handle);
  } else {
    DoSinglePartDownload(handle);
  }
}

// --------------------------------------------------------------------------
bool QSTransferManager::PrepareUpload(
    const shared_ptr<TransferHandle> &handle) {
  assert(GetBufferSize() > 0);
  if (!(GetBufferSize() > 0)) {
    DebugError("Buffer size is less than 0");
    return false;
  }

  bool isRetry = handle->HasParts();
  if (isRetry) {
    for (auto part : handle->GetFailedParts()) {
      handle->AddQueuePart(part.second);
    }
  } else {
    auto totalTransferSize = handle->GetBytesTotalSize();
    if (totalTransferSize >=
        GetUploadMultipartThresholdSize()) {  // multiple upload
      handle->SetIsMultiPart(true);

      bool initSuccess = false;
      string uploadId;
      auto err = GetClient()->InitiateMultipartUpload(handle->GetObjectKey(),
                                                      &uploadId);
      if (IsGoodQSError(err)) {
        handle->SetMultipartId(uploadId);
        initSuccess = true;
      } else {
        handle->SetError(err);
        handle->UpdateStatus(TransferStatus::Failed);
        DebugError(GetMessageForQSError(err));
        initSuccess = false;
      }
      if (!initSuccess) {
        return false;
      }

      size_t partCount = std::ceil(totalTransferSize / GetBufferSize());
      size_t lastCuttingSize =
          totalTransferSize - (partCount - 1) * GetBufferSize();
      bool needAverageLastTwoPart =
          lastCuttingSize < GetUploadMultipartMinPartSize();

      auto count = needAverageLastTwoPart ? partCount - 1 : partCount;
      for (size_t i = 1; i < count; ++i) {
        // part id, best progress in bytes, part size, range begin
        handle->AddQueuePart(make_shared<Part>(
            i, 0, GetBufferSize(),
            handle->GetContentRangeBegin() + (i - 1) * GetBufferSize()));
      }

      size_t sz = needAverageLastTwoPart
                      ? (lastCuttingSize + GetBufferSize()) / 2
                      : lastCuttingSize;
      for (size_t i = count; i <= partCount; ++i) {
        handle->AddQueuePart(make_shared<Part>(
            i, 0, sz,
            handle->GetContentRangeBegin() + (count - 1) * GetBufferSize() +
                (i - count) * sz));
      }
    } else {  // single upload
      handle->SetIsMultiPart(false);
      handle->AddQueuePart(make_shared<Part>(1, 0, totalTransferSize,
                                             handle->GetContentRangeBegin()));
    }
  }
  return true;
}

// --------------------------------------------------------------------------
void QSTransferManager::DoSinglePartUpload(
    const shared_ptr<TransferHandle> &handle) {
  auto queuedParts = handle->GetQueuedParts();
  assert(queuedParts.size() == 1);

  const auto &part = queuedParts.begin()->second;
  handle->AddPendingPart(part);

  auto fileSize = handle->GetBytesTotalSize();
  auto buf = Buffer(new vector<char>(fileSize));
  string objKey = handle->GetObjectKey();
  auto &drive = QS::FileSystem::Drive::Instance();
  auto &cache = drive.GetCache();
  auto &dirTree = drive.GetDirectoryTree();
  auto node = dirTree->Find(objKey).lock();
  assert(node && *node);
  bool success = cache->Read(objKey, 0, fileSize, &(*buf)[0], node);
  if (!success) {
    DebugError("Fail to read cache [file = " + objKey + "], stop upload");
    return;
  }

  auto stream = make_shared<IOStream>(std::move(buf), fileSize);
  auto receivedHandler = [this, handle, part,
                          stream](const ClientError<QSError> &err) {
    if (IsGoodQSError(err)) {
      handle->ChangePartToCompleted(part);  // without eTag, as sdk
                                            // PutObjectOutput not return etag
      handle->UpdateStatus(TransferStatus::Completed);
      auto streamBuf = dynamic_cast<StreamBuf *>(stream->rdbuf());
      if (streamBuf) {
        auto buf = streamBuf->ReleaseBuffer();
        if (buf) {
          buf.reset();
        }
      }
    } else {
      handle->ChangePartToFailed(part);
      handle->UpdateStatus(TransferStatus::Failed);
      handle->SetError(err);
      DebugError(GetMessageForQSError(err));
    }
  };

  GetClient()->GetExecutor()->SubmitAsyncPrioritized(
      receivedHandler, [this, objKey, fileSize, stream]() {
        return GetClient()->UploadFile(objKey, fileSize, stream);
      });
}

// --------------------------------------------------------------------------
void QSTransferManager::DoMultiPartUpload(
    const shared_ptr<TransferHandle> &handle) {
  auto queuedParts = handle->GetQueuedParts();
  string objKey = handle->GetObjectKey();
  auto &drive = QS::FileSystem::Drive::Instance();
  auto &cache = drive.GetCache();
  auto &dirTree = drive.GetDirectoryTree();
  auto node = dirTree->Find(objKey).lock();
  assert(node && *node);

  auto ipart = queuedParts.begin();
  for (; ipart != queuedParts.end() && handle->ShouldContinue(); ++ipart) {
    auto buffer = GetBufferManager()->Acquire();
    if (!buffer) {
      DebugWarning("Unable to acquire resource, stop upload");
      break;
    }

    const auto &part = ipart->second;
    bool success = cache->Read(objKey, part->GetRangeBegin(), GetBufferSize(),
                               &(*buffer)[0], node);
    if (!success) {
      DebugError("Fail to read cache [file:offset:len = " + objKey + ":" +
                 to_string(part->GetRangeBegin()) + ":" +
                 to_string(GetBufferSize()) + "], stop upload");

      GetBufferManager()->Release(std::move(buffer));
      break;
    }

    if (handle->ShouldContinue()) {
      auto stream = make_shared<IOStream>(std::move(buffer), GetBufferSize());
      handle->AddPendingPart(part);

      auto receivedHandler = [this, handle, part,
                              stream](const ClientError<QSError> &err) {
        if (IsGoodQSError(err)) {
          handle->ChangePartToCompleted(part);
        } else {
          handle->ChangePartToFailed(part);
          handle->SetError(err);
          DebugError(GetMessageForQSError(err));
        }

        // release part buffer backe to resouce manager
        auto partStreamBuf = dynamic_cast<StreamBuf *>(stream->rdbuf());
        if (partStreamBuf) {
          GetBufferManager()->Release(partStreamBuf->ReleaseBuffer());
        }

        // update status
        if (!handle->HasPendingParts() && !handle->HasQueuedParts()) {
          if (!handle->HasFailedParts() && handle->DoneTransfer()) {
            // complete multipart upload
            std::set<int> partIds;
            for (auto &idToPart : handle->GetCompletedParts()) {
              partIds.emplace(idToPart.second->GetPartId());
            }
            vector<int> completedPartIds(partIds.begin(), partIds.end());
            auto err = GetClient()->CompleteMultipartUpload(
                handle->GetObjectKey(), handle->GetMultiPartId(),
                completedPartIds);
            if (IsGoodQSError(err)) {
              handle->UpdateStatus(TransferStatus::Completed);
            } else {
              handle->UpdateStatus(TransferStatus::Failed);
              handle->SetError(err);
              DebugError(GetMessageForQSError(err));
            }
          } else {
            handle->UpdateStatus(TransferStatus::Failed);
          }
        }
      };

      string uploadId = handle->GetMultiPartId();
      auto partId = part->GetPartId();
      auto contentLen = GetBufferSize();
      GetClient()->GetExecutor()->SubmitAsync(
          receivedHandler,
          [this, objKey, uploadId, partId, contentLen, stream]() {
            return GetClient()->UploadMultipart(objKey, uploadId, partId,
                                                contentLen, stream);
          });

    } else {
      GetBufferManager()->Release(std::move(buffer));
    }
  }

  for (; ipart != queuedParts.end(); ++ipart) {
    handle->ChangePartToFailed(ipart->second);
  }
}

// --------------------------------------------------------------------------
void QSTransferManager::DoUpload(const shared_ptr<TransferHandle> &handle) {
  handle->UpdateStatus(TransferStatus::InProgress);
  if (!PrepareUpload(handle)) {
    return;
  }
  if (handle->IsMultipart()) {
    DoMultiPartUpload(handle);
  } else {
    DoSinglePartUpload(handle);
  }
}

}  // namespace Client
}  // namespace QS
