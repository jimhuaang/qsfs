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
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
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
#include "client/QSClientImpl.h"
#include "client/QSClientUtils.h"
#include "client/QSError.h"
#include "client/URI.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "data/FileMetaData.h"
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

using QS::Data::BuildDefaultDirectoryMeta;
using QS::Data::Node;
using QS::FileSystem::Drive;
using QS::FileSystem::GetDirectoryMimeType;
using QS::FileSystem::LookupMimeType;
using QS::StringUtils::LTrim;
using QS::StringUtils::RTrim;
using QS::TimeUtils::SecondsToRFC822GMT;
using QS::TimeUtils::RFC822GMTToSeconds;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetBaseName;
using QS::Utils::GetDirName;
using QS::Utils::IsRootDirectory;
using std::chrono::milliseconds;
using std::iostream;
using std::deque;
using std::shared_ptr;
using std::string;
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

}  // namespace

static std::once_flag onceFlagGetClientImpl;
static std::once_flag onceFlagStartService;
unique_ptr<QingStor::QingStorService> QSClient::m_qingStorService = nullptr;

// TODO(jim): add template for retrying code

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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadBucket();
    ++attemptedRetries;
  }

  if(outcome.IsSuccess()){
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
  if(node && *node){
    // In case of hard links, multiple node have the same file, do not delete the
    // file for a hard link.
    if (node->IsHardLink() || (!node->IsDirectory() && node->GetNumLink() >= 2)) {
      dirTree->Remove(filePath);
      return ClientError<QSError>(QSError::GOOD, false);
    }
  }

  auto outcome = GetQSClientImpl()->DeleteObject(filePath);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->DeleteObject(filePath);
    ++attemptedRetries;
  }

  if(outcome.IsSuccess()){
    dirTree->Remove(filePath);
    
    auto &cache = drive.GetCache();
    if (cache && !node->IsDirectory()) {
      cache->Erase(filePath);
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteDirectory(const string &dirPath,
                                               bool recursive) {
  string dir = AppendPathDelim(dirPath);
  ListDirectory(dir);  // will update directory tree

  auto &drive = Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  auto node = dirTree->Find(dir).lock();
  if (!(node && *node)) {
    return ClientError<QSError>(QSError::GOOD, false);
  }
  if (!node->IsDirectory()) {
    return ClientError<QSError>(QSError::ACTION_INVALID, false);
  }

  if(node->IsEmpty()){
    return DeleteFile(dirPath);
  }

  auto err = ClientError<QSError>(QSError::GOOD, false);
  if(recursive){
    auto files = node->GetChildrenIdsRecursively();
    while(!files.empty()){
      auto file = files.front();
      files.pop_front();
      err = DeleteFile(file);
      if(!IsGoodQSError(err)){
        break;
      }
    }
  }
  
  return err;
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(filePath, &input);
    ++attemptedRetries;
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

  auto outcome = GetQSClientImpl()->PutObject(dirPath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(dirPath, &input);
    ++attemptedRetries;
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
  // TODO(jim):
  PutObjectInput input;
  input.SetXQSMoveSource(BuildXQSSourceString(sourceFilePath));
  //input.SetContentType(LookupMimeType(destFilePath));

  auto outcome = GetQSClientImpl()->PutObject(destFilePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(destFilePath, &input);
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto &drive = Drive::Instance();
    auto &dirTree = drive.GetDirectoryTree();
    if (dirTree) {
      dirTree->Rename(sourceFilePath, destFilePath);
    }
    auto &cache = drive.GetCache();
    if (cache) {
      cache->Rename(sourceFilePath, destFilePath);
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MoveDirectory(const string &sourceDirPath,
                                             const string &targetDirPath) {
  string sourceDir = AppendPathDelim(sourceDirPath);
  ListDirectory(sourceDir);  // will update directory tree

  auto &drive = Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  auto node = dirTree->Find(sourceDir).lock();
  if (!(node && *node)) {
    return ClientError<QSError>(QSError::GOOD, false);
  }
  if (!node->IsDirectory()) {
    return ClientError<QSError>(QSError::ACTION_INVALID, false);
  }

  auto childPaths = node->GetChildrenIdsRecursively();
  size_t len = sourceDir.size();
  string targetDir = AppendPathDelim(targetDirPath);
  deque<string> childTargetPaths;
  for (auto &path : childPaths) {
    if (path.substr(0, len) != sourceDir) {
      DebugError("Directory " + sourceDir + " has an invalid child file " +
                 path);
      return ClientError<QSError>(QSError::PARAMETER_VALUE_INAVLID, false);
    }
    childTargetPaths.emplace_back(targetDir + path.substr(len));
  }

  // Move children
  auto err = ClientError<QSError>(QSError::GOOD, false);
  while (!childPaths.empty() && !childTargetPaths.empty()) {
    auto source = childPaths.back();
    childPaths.pop_back();
    auto target = childTargetPaths.back();
    childTargetPaths.pop_back();

    err = MoveFile(source, target);
    if (!IsGoodQSError(err)) {
      break;
    }
  }

  // Move dir itself
  err = MoveFile(sourceDir, targetDir);

  return err;
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DownloadFile(const string &filePath,
                                            const string &range,
                                            const shared_ptr<iostream> &buffer,
                                            string *eTag) {
  GetObjectInput input;
  if (!range.empty()) {
    input.SetRange(range);
  }

  auto outcome = GetQSClientImpl()->GetObject(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetObject(filePath, &input);
    ++attemptedRetries;
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->InitiateMultipartUpload(filePath, &input);
    ++attemptedRetries;
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

  auto outcome = GetQSClientImpl()->UploadMultipart(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->UploadMultipart(filePath, &input);
    ++attemptedRetries;
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->CompleteMultipartUpload(filePath, &input);
    ++attemptedRetries;
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->AbortMultipartUpload(filePath, &input);
    ++attemptedRetries;
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

  auto outcome = GetQSClientImpl()->PutObject(filePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(filePath, &input);
    ++attemptedRetries;
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

  auto &drive = QS::FileSystem::Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  assert(dirTree);
  auto dirNode = drive.GetNodeSimple(dirPath).lock();
  bool doUpdate = false;
  if(dirNode && *dirNode){
    if (IsRootDirectory(dirPath)) {
      // For root, as we cannot know if bucket is modified or not using
      // current SDK, so we only update it if it's empty.
      doUpdate = dirNode->IsEmpty();
    } else {
      HeadObjectInput input;
      input.SetIfModifiedSince(SecondsToRFC822GMT(dirNode->GetMTime()));
      auto out = GetQSClientImpl()->HeadObject(dirPath, &input);
  
      if (out.IsSuccess()) {
        auto res = out.GetResult();
        if (res.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
          // As for object storage, there is no concept of directory.
          // For some case, such as
          //   an object of "/abc/tst.txt" can exist without existing
          //   object of "/abc/"
          // For this case we just update the directory if it's empty.
          doUpdate = dirNode->IsEmpty();
        } else if (res.GetResponseCode() == HttpResponseCode::NOT_MODIFIED) {
          doUpdate = false;
        } else {  // modified
          doUpdate = true;
        }
      } else {
        doUpdate = false;
        DebugError(GetMessageForQSError(out.GetError()));
      }
    }
  }

  ListObjectsInput listObjInput;
  listObjInput.SetLimit(Constants::BucketListObjectsLimit);
  listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
  if (!IsRootDirectory(dirPath)) {
    string prefix = AppendPathDelim(LTrim(dirPath, '/'));
    listObjInput.SetPrefix(prefix);
  }

  bool resultTruncated = false;
  // Set maxCount for a single list operation.
  // This will request for ListObjects seperately, so we can construct
  // directory tree gradually. This will be helpful for the performance
  // if there are a huge number of objects to list.
  uint64_t maxCount = static_cast<uint64_t>(Constants::BucketListObjectsLimit);
  do {
    auto outcome = GetQSClientImpl()->ListObjects(&listObjInput,
                                                  &resultTruncated, maxCount);
    unsigned attemptedRetries = 0;
    while (
        !outcome.IsSuccess() &&
        GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
      int32_t sleepMilliseconds =
          GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                           attemptedRetries);
      RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
      outcome = GetQSClientImpl()->ListObjects(&listObjInput, &resultTruncated,
                                               maxCount);
      ++attemptedRetries;
    }

    if (!outcome.IsSuccess()) {
      return outcome.GetError();
    }

    for (auto &listObjOutput : outcome.GetResult()) {
      if (!(dirNode && *dirNode)) {  // directory not existing at this moment
        // Add its children to dir tree
        auto fileMetaDatas = QSClientUtils::ListObjectsOutputToFileMetaDatas(
            listObjOutput, true);  // add dir itself
        dirTree->Grow(std::move(fileMetaDatas));
      } else if (doUpdate) {  // directory existing
        auto fileMetaDatas = QSClientUtils::ListObjectsOutputToFileMetaDatas(
            listObjOutput, false);  // not add dir itself
        if (dirNode->IsEmpty()) {
          dirTree->Grow(std::move(fileMetaDatas));
        } else {
          dirTree->UpdateDiretory(dirPath, std::move(fileMetaDatas));
        }
      }
    }  // for list object output

  } while (resultTruncated);
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Stat(const string &path, time_t modifiedSince,
                                    bool *modified) {
  if (modified != nullptr) {
    *modified = false;
  }

  if (IsRootDirectory(path)) {
    // As headobject will not return any meta for root "/",
    // so we head bucket instead, and also head bucket return no meta, so we
    // cannot know if bucket is modified or not, so we just think bucket is
    // not modified, but this may need update if SDK return more data in future.
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadObject(path, &input);
    ++attemptedRetries;
  }

  auto &dirTree = Drive::Instance().GetDirectoryTree();
  assert(dirTree);
  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    if (res.GetResponseCode() == HttpResponseCode::NOT_MODIFIED) {
      // if is not modified, no meta is returned, so just return directly
      return ClientError<QSError>(QSError::GOOD, false);
    }

    if (res.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
      // As for object storage, there is no concept of directory.
      // For some case, such as
      //   an object of "/abc/tst.txt" can exist without existing
      //   object of "/abc/"
      // In this case, headobject with objKey of "/abc/" will not get

      // So, we need to use listobject with prefix of "/abc/" to confirm a
      // directory is actually needed.
      if (path.back() == '/') {
        ListObjectsInput listObjInput;
        listObjInput.SetLimit(10);
        listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
        listObjInput.SetPrefix(LTrim(path, '/'));
        auto outcome =
            GetQSClientImpl()->ListObjects(&listObjInput, nullptr, 10);

        if (outcome.IsSuccess()) {
          *modified = true;
          dirTree->Grow(BuildDefaultDirectoryMeta(path));  // add dir node
          return ClientError<QSError>(QSError::GOOD, false);
        }
      }

      DebugInfo("Object (" + path + ") Not Found ");
      return ClientError<QSError>(QSError::GOOD, false);
    }

    *modified = true;
    auto fileMetaData =
        QSClientUtils::HeadObjectOutputToFileMetaData(path, res);
    if (fileMetaData) {
      dirTree->Grow(std::move(fileMetaData));  // add Node to dir tree
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
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
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetBucketStatistics();
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    QSClientUtils::GetBucketStatisticsOutputToStatvfs(res, stvfs);
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
