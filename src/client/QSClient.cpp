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

#include "client/QSClient.h"

#include <assert.h>
#include <stdint.h>  // for uint64_t

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/HttpCommon.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"
#include "qingstor-sdk-cpp/types/ObjectPartType.h"

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
#include "client/Constants.h"
#include "client/Protocol.h"
#include "client/QSClientConverter.h"
#include "client/QSClientImpl.h"
#include "client/QSError.h"
#include "client/URI.h"
#include "client/Utils.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "data/FileMetaData.h"
#include "data/Size.h"
#include "filesystem/Drive.h"
#include "filesystem/MimeTypes.h"

namespace QS {

namespace Client {

using QingStor::AbortMultipartUploadInput;
using QingStor::Bucket;
using QingStor::CompleteMultipartUploadInput;
using QingStor::GetObjectInput;
using QingStor::HeadObjectInput;
using QingStor::Http::HttpResponseCode;
using QingStor::InitiateMultipartUploadInput;
using QingStor::ListObjectsInput;
using QingStor::PutObjectInput;
using QingStor::QingStorService;
using QingStor::QsConfig;  // sdk config
using QingStor::UploadMultipartInput;

using QS::Client::Utils::ParseRequestContentRange;
using QS::Data::BuildDefaultDirectoryMeta;
using QS::Data::Node;
using QS::FileSystem::Drive;
using QS::FileSystem::GetDirectoryMimeType;
using QS::FileSystem::GetSymlinkMimeType;
using QS::FileSystem::LookupMimeType;
using QS::StringUtils::FormatPath;
using QS::StringUtils::LTrim;
using QS::StringUtils::RTrim;
using QS::TimeUtils::SecondsToRFC822GMT;
using QS::TimeUtils::RFC822GMTToSeconds;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetBaseName;
using QS::Utils::GetDirName;
using QS::Utils::IsRootDirectory;
using std::chrono::milliseconds;
using std::make_shared;
using std::iostream;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::unique_ptr;

namespace {

// --------------------------------------------------------------------------
string BuildXQSSourceString(const string &objKey) {
  const auto &clientConfig = ClientConfiguration::Instance();
  // format: /bucket-name/object-key
  string ret = "/";
  ret.append(clientConfig.GetBucket());
  ret.append("/");
  ret.append(LTrim(objKey, '/'));
  return ret;
}

// --------------------------------------------------------------------------
uint32_t CalculateTransferTimeForFile(uint64_t fileSize) {
  // 2000 milliseconds per MB1
  return std::ceil(static_cast<long double>(fileSize) / QS::Data::Size::MB1) *
             2000 + 1000;
}

// --------------------------------------------------------------------------
uint32_t CalculateTimeForListObjects(uint64_t maxCount) {
  // 1000 milliseconds per 200 objects
  return std::ceil(static_cast<long double>(maxCount) / 200) * 1000 + 1000;
}

}  // namespace

static std::once_flag onceFlagGetClientImpl;
static std::once_flag onceFlagStartService;
unique_ptr<QingStor::QingStorService> QSClient::m_qingStorService = nullptr;

// --------------------------------------------------------------------------
QSClient::QSClient() : Client() {
  StartQSService();
  InitializeClientImpl();
}

// --------------------------------------------------------------------------
QSClient::~QSClient() { CloseQSService(); }

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::HeadBucket() {
  auto outcome = GetQSClientImpl()->HeadBucket();
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadBucket();
    ++attemptedRetries;
    DebugInfo("Retry head bucket");
  }

  if (outcome.IsSuccess()) {
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
// DeleteFile is used to delete a file or an empty directory.
ClientError<QSError> QSClient::DeleteFile(const string &filePath) {
  auto &drive = Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  assert(dirTree);
  auto node = dirTree->Find(filePath).lock();
  if (node && *node) {
    // In case of hard links, multiple node have the same file, do not delete
    // the file for a hard link.
    if (node->IsHardLink() ||
        (!node->IsDirectory() && node->GetNumLink() >= 2)) {
      dirTree->Remove(filePath);
      return ClientError<QSError>(QSError::GOOD, false);
    }
  }

  auto err = DeleteObject(filePath);
  if (IsGoodQSError(err)) {
    dirTree->Remove(filePath);

    auto &cache = drive.GetCache();
    if (cache && cache->HasFile(filePath)) {
      cache->Erase(filePath);
    }
  }

  return err;
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteObject(const std::string &filePath) {
  auto outcome = GetQSClientImpl()->DeleteObject(filePath);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->DeleteObject(filePath);
    ++attemptedRetries;
    DebugInfo("Retry delete object " + FormatPath(filePath));
  }

  return outcome.IsSuccess() ? ClientError<QSError>(QSError::GOOD, false)
                             : outcome.GetError();
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MakeFile(const string &filePath) {
  PutObjectInput input;
  input.SetContentLength(0);  // create empty file
  input.SetContentType(LookupMimeType(filePath));

  auto outcome = GetQSClientImpl()->PutObject(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(filePath, &input);
    ++attemptedRetries;
    DebugInfo("Retry make file " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    // As sdk doesn't return the created file meta data in PutObjectOutput,
    // So we cannot grow the directory tree here, instead we need to call
    // Stat to head the object again in Drive::MakeFile;
    //
    // auto &drive = Drive::Instance();
    // auto &dirTree = Drive::Instance().GetDirectoryTree();
    // if (dirTree) {
    //   dirTree->Grow(PutObjectOutputToFileMeta());  // no implementation
    // }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MakeDirectory(const string &dirPath) {
  PutObjectInput input;
  input.SetContentLength(0);  // directory has zero length
  input.SetContentType(GetDirectoryMimeType());
  auto dir = AppendPathDelim(dirPath);

  auto outcome = GetQSClientImpl()->PutObject(dir, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(dir, &input);
    ++attemptedRetries;
    DebugInfo("Retry make directory " + FormatPath(dirPath));
  }

  if (outcome.IsSuccess()) {
    // As sdk doesn't return the created file meta data in PutObjectOutput,
    // So we cannot grow the directory tree here, instead we need to call
    // Stat to head the object again in Drive::MakeDir;
    //
    // auto &drive = Drive::Instance();
    // auto &dirTree = Drive::Instance().GetDirectoryTree();
    // if (dirTree) {
    //   dirTree->Grow(PutObjectOutputToFileMeta());  // no implementation
    // }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MoveFile(const string &sourceFilePath,
                                        const string &destFilePath) {
  auto err = MoveObject(sourceFilePath, destFilePath);
  auto &drive = Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  if (IsGoodQSError(err)) {
    if (dirTree && dirTree->Has(sourceFilePath)) {
      dirTree->Rename(sourceFilePath, destFilePath);
    }
    auto &cache = drive.GetCache();
    if (cache && cache->HasFile(sourceFilePath)) {
      cache->Rename(sourceFilePath, destFilePath);
    }
  } else {
    // Handle following special case
    // As for object storage, there is no concept of directory.
    // For some case, such as
    //   an object of "/abc/tst.txt" can exist without existing
    //   object of "/abc/"
    // In this case, putobject(move) with objKey of "/abc/" will not success.
    // So, we need to create it.
    bool isDir = RTrim(sourceFilePath, ' ').back() == '/';
    if (err.GetError() == QSError::KEY_NOT_EXIST && isDir) {
      auto err1 = MakeDirectory(destFilePath);
      if (IsGoodQSError(err1)) {
        if (dirTree && dirTree->Has(sourceFilePath)) {
          dirTree->Rename(sourceFilePath, destFilePath);
        }
      } else {
        DebugInfo("Object not created : " + GetMessageForQSError(err1) +
                  FormatPath(destFilePath));
      }
    }
  }

  return err;
}

// --------------------------------------------------------------------------
// Notes: MoveDirectory will do nothing on dir tree and cache.
ClientError<QSError> QSClient::MoveDirectory(const string &sourceDirPath,
                                             const string &targetDirPath,
                                             bool async) {
  string sourceDir = AppendPathDelim(sourceDirPath);
  // List the source directory all objects
  auto outcome = ListObjects(sourceDir);
  if (!outcome.IsSuccess()) {
    DebugError("Fail to list objects " + FormatPath(sourceDir));
    return outcome.GetError();
  }

  auto receivedHandler = [](const ClientError<QSError> &err) {
    DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
  };

  string targetDir = AppendPathDelim(targetDirPath);
  size_t lenSourceDir = sourceDir.size();
  auto &listObjOutputs = outcome.GetResult();

  // move sub files
  auto prefix = LTrim(sourceDir, '/');
  for (auto &listObjOutput : listObjOutputs) {
    for (auto &key : listObjOutput.GetKeys()) {
      // sdk will put dir (if exists) itself into keys, ignore it
      if (prefix == key.GetKey()) {
        continue;
      }
      auto sourceSubFile = "/" + key.GetKey();
      auto targetSubFile = targetDir + sourceSubFile.substr(lenSourceDir);

      if (async) {  // asynchronizely
        GetExecutor()->SubmitAsync(
            receivedHandler, [this, sourceSubFile, targetSubFile] {
              return MoveObject(sourceSubFile, targetSubFile);
            });
      } else {  // synchronizely
        receivedHandler(MoveObject(sourceSubFile, targetSubFile));
      }
    }
  }

  // move sub folders
  for (auto &listObjOutput : listObjOutputs) {
    for (const auto &commonPrefix : listObjOutput.GetCommonPrefixes()) {
      auto sourceSubDir = AppendPathDelim("/" + commonPrefix);
      auto targetSubDir = targetDir + sourceSubDir.substr(lenSourceDir);

      if (async) {  // asynchronizely
        GetExecutor()->SubmitAsync(
            receivedHandler, [this, sourceSubDir, targetSubDir] {
              return MoveDirectory(sourceSubDir, targetSubDir);
            });
      } else {  // synchronizely
        receivedHandler(MoveDirectory(sourceSubDir, targetSubDir));
      }
    }
  }

  // move dir itself
  if (async) {  // asynchronizely
    GetExecutor()->SubmitAsync(receivedHandler, [this, sourceDir, targetDir] {
      return MoveObject(sourceDir, targetDir);
    });
  } else {  // synchronizely
    receivedHandler(MoveObject(sourceDir, targetDir));
  }

  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MoveObject(const std::string &sourcePath,
                                          const std::string &targetPath) {
  PutObjectInput input;
  input.SetXQSMoveSource(BuildXQSSourceString(sourcePath));
  // sdk cpp require content-length parameter, though it will be ignored
  // so, set 0 to avoid sdk parameter checking failure
  input.SetContentLength(0);
  // it seems put-move discards the content-type, so we set directory
  // mime type explicitly
  bool isDir = RTrim(sourcePath, ' ').back() == '/';
  if (isDir) {
    input.SetContentType(GetDirectoryMimeType());
  }

  // Seems move object cost more time than head object, so set more time
  auto timeDuration =
      ClientConfiguration::Instance().GetTransactionTimeDuration() * 5;

  auto outcome = GetQSClientImpl()->PutObject(targetPath, &input, timeDuration);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(targetPath, &input, timeDuration);
    ++attemptedRetries;
    DebugInfo("Retry move object " + FormatPath(sourcePath, targetPath));
  }

  if (outcome.IsSuccess()) {
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    // For move object, if retry happens, and if the previous transaction
    // success finally, this will cause the following retry transaction fail.
    // So we handle the special case, if retry happens and response code is
    // NOT_FOUND(404) then we know the previous transaction before retry success
    auto err = outcome.GetError();
    if (attemptedRetries > 0 && err.GetError() == QSError::KEY_NOT_EXIST) {
      return ClientError<QSError>(QSError::GOOD, false);
    } else {
      return err;
    }
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DownloadFile(const string &filePath,
                                            const shared_ptr<iostream> &buffer,
                                            const string &range, string *eTag) {
  GetObjectInput input;
  uint32_t timeDuration = ClientConfiguration::Instance()
                              .GetTransactionTimeDuration();  // milliseconds
  if (!range.empty()) {
    input.SetRange(range);
    timeDuration =
        CalculateTransferTimeForFile(ParseRequestContentRange(range).second);
  }

  auto outcome = GetQSClientImpl()->GetObject(filePath, &input, timeDuration);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetObject(filePath, &input, timeDuration);
    ++attemptedRetries;
    DebugInfo("Retry download file " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    auto bodyStream = res.GetBody();
    bodyStream->seekg(0, std::ios_base::beg);
    buffer->seekp(0, std::ios_base::beg);
    (*buffer) << bodyStream->rdbuf();
    if (eTag != nullptr) {
      *eTag = res.GetETag();
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::InitiateMultipartUpload(const string &filePath,
                                                       string *uploadId) {
  InitiateMultipartUploadInput input;
  input.SetContentType(LookupMimeType(filePath));

  auto outcome = GetQSClientImpl()->InitiateMultipartUpload(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->InitiateMultipartUpload(filePath, &input);
    ++attemptedRetries;
    DebugInfo("Retry initiate multipart upload " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    if (uploadId != nullptr) {
      *uploadId = res.GetUploadID();
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::UploadMultipart(
    const std::string &filePath, const std::string &uploadId, int partNumber,
    uint64_t contentLength, const std::shared_ptr<std::iostream> &buffer) {
  UploadMultipartInput input;
  input.SetUploadID(uploadId);
  input.SetPartNumber(partNumber);
  input.SetContentLength(contentLength);
  input.SetBody(buffer);

  auto timeDuration = CalculateTransferTimeForFile(contentLength);
  auto outcome =
      GetQSClientImpl()->UploadMultipart(filePath, &input, timeDuration);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome =
        GetQSClientImpl()->UploadMultipart(filePath, &input, timeDuration);
    ++attemptedRetries;
    DebugInfo("Retry upload multipart " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::CompleteMultipartUpload(
    const std::string &filePath, const std::string &uploadId,
    const std::vector<int> &sortedPartIds) {
  CompleteMultipartUploadInput input;
  input.SetUploadID(uploadId);
  std::vector<ObjectPartType> objParts;
  for (auto partId : sortedPartIds) {
    ObjectPartType part;
    part.SetPartNumber(partId);
    objParts.push_back(std::move(part));
  }
  input.SetObjectParts(std::move(objParts));

  auto outcome = GetQSClientImpl()->CompleteMultipartUpload(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->CompleteMultipartUpload(filePath, &input);
    ++attemptedRetries;
    DebugInfo("Retry complete multipart upload " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::AbortMultipartUpload(const string &filePath,
                                                    const string &uploadId) {
  AbortMultipartUploadInput input;
  input.SetUploadID(uploadId);

  auto outcome = GetQSClientImpl()->AbortMultipartUpload(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->AbortMultipartUpload(filePath, &input);
    ++attemptedRetries;
    DebugInfo("Retry abort mulitpart upload " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::UploadFile(const string &filePath,
                                          uint64_t fileSize,
                                          const shared_ptr<iostream> &buffer) {
  PutObjectInput input;
  input.SetContentLength(fileSize);
  input.SetContentType(LookupMimeType(filePath));
  input.SetBody(buffer);

  auto timeDuration = CalculateTransferTimeForFile(fileSize);

  auto outcome = GetQSClientImpl()->PutObject(filePath, &input, timeDuration);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(filePath, &input, timeDuration);
    ++attemptedRetries;
    DebugInfo("Retry upload file " + FormatPath(filePath));
  }

  if (outcome.IsSuccess()) {
    // As sdk doesn't return the created file meta data in PutObjectOutput,
    // So we cannot grow the directory tree here, instead we need to call
    // Stat to head the object again in Drive::MakeFile;
    //
    // auto &drive = Drive::Instance();
    // auto &dirTree = Drive::Instance().GetDirectoryTree();
    // if (dirTree) {
    //   dirTree->Grow(PutObjectOutputToFileMeta());  // no implementation
    // }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::SymLink(const string &filePath,
                                       const string &linkPath) {
  PutObjectInput input;
  input.SetContentLength(filePath.size());
  input.SetContentType(GetSymlinkMimeType());
  auto ss = make_shared<stringstream>(filePath);
  input.SetBody(ss);

  auto outcome = GetQSClientImpl()->PutObject(linkPath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(linkPath, &input);
    ++attemptedRetries;
    DebugInfo("Retry symlink " + FormatPath(filePath, linkPath));
  }

  if (outcome.IsSuccess()) {
    // As sdk doesn't return the created file meta data in PutObjectOutput,
    // So we cannot grow the directory tree here, instead we need to call
    // Stat to head the object again in Drive::MakeFile;
    //
    // auto &drive = Drive::Instance();
    // auto &dirTree = Drive::Instance().GetDirectoryTree();
    // if (dirTree) {
    //   dirTree->Grow(PutObjectOutputToFileMeta());  // no implementation
    // }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::ListDirectory(const string &dirPath) {
  bool resultTruncated = false;
  // Set maxCount for a single list operation.
  // This will request for ListObjects seperately, so we can construct
  // directory tree gradually. This will be helpful for the performance
  // if there are a huge number of objects to list.
  uint64_t maxCount = static_cast<uint64_t>(Constants::BucketListObjectsLimit);

  auto &drive = QS::FileSystem::Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  assert(dirTree);
  auto dirNode = drive.GetNodeSimple(dirPath).lock();
  do {
    auto outcome = ListObjects(dirPath, &resultTruncated, maxCount);
    if (!outcome.IsSuccess()) {
      return outcome.GetError();
    }

    for (auto &listObjOutput : outcome.GetResult()) {
      if (!(dirNode && *dirNode)) {  // directory not existing at this moment
        // Add its children to dir tree
        auto fileMetaDatas =
            QSClientConverter::ListObjectsOutputToFileMetaDatas(
                listObjOutput, true);  // add dir itself
        dirTree->Grow(std::move(fileMetaDatas));
      } else {  // directory existing
        auto fileMetaDatas =
            QSClientConverter::ListObjectsOutputToFileMetaDatas(
                listObjOutput, false);  // not add dir itself
        if (dirNode->IsEmpty()) {
          dirTree->Grow(std::move(fileMetaDatas));
        } else {
          dirTree->UpdateDirectory(dirPath, std::move(fileMetaDatas));
        }
      }
    }  // for list object output
  } while (resultTruncated);

  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ListObjectsOutcome QSClient::ListObjects(const string &dirPath,
                                         bool *resultTruncated,
                                         uint64_t maxCount) {
  ListObjectsInput listObjInput;
  listObjInput.SetLimit(Constants::BucketListObjectsLimit);
  listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
  string prefix = IsRootDirectory(dirPath)
                      ? string()
                      : AppendPathDelim(LTrim(dirPath, '/'));
  listObjInput.SetPrefix(prefix);

  auto timeDuration = CalculateTimeForListObjects(maxCount);

  auto outcome = GetQSClientImpl()->ListObjects(&listObjInput, resultTruncated,
                                                maxCount, timeDuration);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->ListObjects(&listObjInput, resultTruncated,
                                             maxCount, timeDuration);
    ++attemptedRetries;
    DebugInfo("Retry list objects " + FormatPath(dirPath));
  }

  return outcome;
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Stat(const string &path, time_t modifiedSince,
                                    bool *modified) {
  if (modified != nullptr) {
    *modified = false;
  }

  if (IsRootDirectory(path)) {
    // Stat aims to get object meta data. As in object storage, bucket has no
    // data to record last modified time.
    // Bucket mtime is set when connect to it at the first time.
    // We just think bucket mtime is not modified since then.
    return HeadBucket();
  }

  HeadObjectInput input;
  if (modifiedSince >= 0) {
    input.SetIfModifiedSince(SecondsToRFC822GMT(modifiedSince));
  }

  auto outcome = GetQSClientImpl()->HeadObject(path, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadObject(path, &input);
    ++attemptedRetries;
    DebugInfo("Retry head object " + FormatPath(path));
  }

  auto &dirTree = Drive::Instance().GetDirectoryTree();
  assert(dirTree);
  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    if (res.GetResponseCode() == HttpResponseCode::NOT_MODIFIED) {
      // if is not modified, no meta is returned, so just return directly
      return ClientError<QSError>(QSError::GOOD, false);
    }

    if (modified != nullptr) {
      *modified = true;
    }
    auto fileMetaData =
        QSClientConverter::HeadObjectOutputToFileMetaData(path, res);
    if (fileMetaData) {
      dirTree->Grow(std::move(fileMetaData));  // add/update node in dir tree
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    // Handle following special case.
    // As for object storage, there is no concept of directory.
    // For some case, such as
    //   an object of "/abc/tst.txt" can exist without existing
    //   object of "/abc/"
    // In this case, headobject with objKey of "/abc/" will not success.
    // So, we need to use listobject with prefix of "/abc/" to confirm if a
    // directory node is actually needed to be construct in dir tree.
    auto err = outcome.GetError();
    if (err.GetError() == QSError::KEY_NOT_EXIST) {
      if (path.back() == '/') {
        ListObjectsInput listObjInput;
        listObjInput.SetLimit(10);
        listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
        listObjInput.SetPrefix(LTrim(path, '/'));
        auto outcome =
            GetQSClientImpl()->ListObjects(&listObjInput, nullptr, 10);

        if (outcome.IsSuccess()) {
          bool dirExist = false;
          for (auto &listObjOutput : outcome.GetResult()) {
            if (!listObjOutput.GetKeys().empty() ||
                !listObjOutput.GetCommonPrefixes().empty()) {
              dirExist = true;
              break;
            }
          }

          if (dirExist) {
            if (modified != nullptr) {
              *modified = true;
            }
            dirTree->Grow(BuildDefaultDirectoryMeta(path));  // add dir node
            return ClientError<QSError>(QSError::GOOD, false);
          }  // if dir exist
        }    // if outcome is success
      }      // if path is dir
    }

    return err;
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Statvfs(struct statvfs *stvfs) {
  assert(stvfs != nullptr);
  if (stvfs == nullptr) {
    DebugError("Null statvfs parameter");
    return ClientError<QSError>(QSError::PARAMETER_MISSING, false);
  }

  auto outcome = GetQSClientImpl()->GetBucketStatistics();
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    uint32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetBucketStatistics();
    ++attemptedRetries;
    DebugInfo("Retry get bucket statistics");
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    QSClientConverter::GetBucketStatisticsOutputToStatvfs(res, stvfs);
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
const unique_ptr<QingStorService> &QSClient::GetQingStorService() {
  StartQSService();
  return m_qingStorService;
}

// --------------------------------------------------------------------------
const shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() const {
  return const_cast<QSClient *>(this)->GetQSClientImpl();
}

// --------------------------------------------------------------------------
shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() {
  std::call_once(onceFlagGetClientImpl, [this] {
    auto r = GetClientImpl();
    assert(r);
    FatalIf(!r, "QSClient is initialized with null QSClientImpl");
    auto p = dynamic_cast<QSClientImpl *>(r.get());
    m_qsClientImpl = std::move(shared_ptr<QSClientImpl>(r, p));
  });
  return m_qsClientImpl;
}

// --------------------------------------------------------------------------
void QSClient::StartQSService() {
  std::call_once(onceFlagStartService, [] {
    const auto &clientConfig = ClientConfiguration::Instance();
    // Must set sdk log at beginning, otherwise sdk will broken due to
    // uninitialization of plog sdk depended on.
    // QingStor SDK need a log dir.
    QingStorService::initService(GetDirName(clientConfig.GetClientLogFile()));

    static QsConfig sdkConfig(clientConfig.GetAccessKeyId(),
                              clientConfig.GetSecretKey());
    sdkConfig.m_LogLevel =
        GetClientLogLevelName(clientConfig.GetClientLogLevel());
    sdkConfig.m_AdditionalUserAgent = clientConfig.GetAdditionalAgent();
    sdkConfig.m_Host = Http::HostToString(clientConfig.GetHost());
    sdkConfig.m_Protocol = Http::ProtocolToString(clientConfig.GetProtocol());
    sdkConfig.m_Port = clientConfig.GetPort();
    sdkConfig.m_ConnectionRetries = clientConfig.GetConnectionRetries();
    m_qingStorService =
        unique_ptr<QingStorService>(new QingStorService(sdkConfig));
  });
}

// --------------------------------------------------------------------------
void QSClient::CloseQSService() { QingStorService::shutdownService(); }

// --------------------------------------------------------------------------
void QSClient::InitializeClientImpl() {
  const auto &clientConfig = ClientConfiguration::Instance();
  if (GetQSClientImpl()->GetBucket()) return;
  GetQSClientImpl()->SetBucket(unique_ptr<Bucket>(
      new Bucket(m_qingStorService->GetConfig(), clientConfig.GetBucket(),
                 clientConfig.GetZone())));
}

}  // namespace Client
}  // namespace QS
