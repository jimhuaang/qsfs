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
#include <string>
#include <utility>

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

namespace QS {

namespace Client {

using QS::Client::Utils::BuildRequestRange;
using QS::Data::IOStream;
using QS::Data::StreamBuf;
using std::iostream;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;

shared_ptr<TransferHandle> QSTransferManager::UploadFile() { return nullptr; }
shared_ptr<TransferHandle> QSTransferManager::RetryUpload() { return nullptr; }
void QSTransferManager::UploadDirectory(const std::string &directory) {}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::DownloadFile(
    const string &filePath, off_t offset, size_t size,
    shared_ptr<iostream> bufStream) {
  // Drive::ReadFile has checked the object existence, so no check here.
  // Drive::ReadFile has already ajust the download size, so no ajust here.
  if(!bufStream){
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
void QSTransferManager::DownloadDirectory(const std::string &directory) {}

void QSTransferManager::AbortMultipartUpload(
    const shared_ptr<TransferHandle> &handle) {}

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

}  // namespace Client
}  // namespace QS
