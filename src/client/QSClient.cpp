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

using QS::Data::Node;
using QS::FileSystem::Drive;
using QS::FileSystem::GetDirectoryMimeType;
using QS::FileSystem::LookupMimeType;
using QS::StringUtils::LTrim;
using QS::TimeUtils::SecondsToRFC822GMT;
using QS::TimeUtils::RFC822GMTToSeconds;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetBaseName;
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

// --------------------------------------------------------------------------
string FilePathToObjectKey(const string& filePath){
  // object key should be not leading with '/'
  return LTrim(LTrim(filePath, ' '), '/');
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->DeleteObject(objKey);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->DeleteObject(objKey);
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


  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->PutObject(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(objKey, &input);
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

  auto objKey = FilePathToObjectKey(dirPath);
  auto outcome = GetQSClientImpl()->PutObject(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(objKey, &input);
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
  input.SetXQSMoveSource(
      BuildXQSSourceString(FilePathToObjectKey(sourceFilePath)));
  //input.SetContentType(LookupMimeType(destFilePath));

  auto destObjKey = FilePathToObjectKey(destFilePath);
  auto outcome = GetQSClientImpl()->PutObject(destObjKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(destObjKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->GetObject(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetObject(objKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->InitiateMultipartUpload(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->InitiateMultipartUpload(objKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->UploadMultipart(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->UploadMultipart(objKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->CompleteMultipartUpload(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->CompleteMultipartUpload(objKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->AbortMultipartUpload(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->AbortMultipartUpload(objKey, &input);
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

  auto objKey = FilePathToObjectKey(filePath);
  auto outcome = GetQSClientImpl()->PutObject(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(objKey, &input);
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
  ListObjectsInput listObjInput;
  listObjInput.SetLimit(Constants::BucketListObjectsLimit);
  listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
  if (!QS::Utils::IsRootDirectory(dirPath)) {
    string prefix = AppendPathDelim(FilePathToObjectKey(dirPath));
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

    if (outcome.IsSuccess()) {
      for (auto &listObjOutput : outcome.GetResult()) {
        auto fileMetaDatas = QSClientUtils::ListObjectsOutputToFileMetaDatas(
            listObjOutput, true);  // add dir itself
        auto &drive = QS::FileSystem::Drive::Instance();
        auto &dirTree = drive.GetDirectoryTree();
        if (!dirTree) break;
        // DO NOT invoking update directory rescursively
        auto res = drive.GetNode(dirPath, false);
        auto dirNode = res.first.lock();
        bool modified = res.second;
        if (dirNode && modified) {
          dirTree->UpdateDiretory(dirPath, std::move(fileMetaDatas));
        } else {  // directory not existing at this moment
          // Add its children to dir tree
          dirTree->Grow(std::move(fileMetaDatas));
        }
      }
    } else {
      return outcome.GetError();
    }
  } while (resultTruncated);
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Stat(const string &path, time_t modifiedSince,
                                    bool *modified) {
  if (modified != nullptr) {
    *modified = false;
  }

  if(QS::Utils::IsRootDirectory(path)){
    return HeadBucket();
  }

  HeadObjectInput input;
  if (modifiedSince > 0) {
    input.SetIfModifiedSince(SecondsToRFC822GMT(modifiedSince));
  }

  auto objKey = FilePathToObjectKey(path);
  auto outcome = GetQSClientImpl()->HeadObject(objKey, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadObject(objKey, &input);
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    /*     if (res.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
          // As for object storage, there is no concept of directory.
          // For some case, such as
          //   an object of "/abc/tst.txt" can exist without existing
          //   object of "/abc/"
          // In this case, headobject with objKey of "/abc/" will not get
       response.
          // So, we need to use listobject with prefix of "/abc/" to confirm a
          // directory is actually needed.
          // ListDirectory(path); recursively TODO(jim):
        } else  */
    if (res.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
      return ClientError<QSError>(
          QSError::KEY_NOT_EXIST, "HeadObject " + path,
          SDKResponseCodeToString(HttpResponseCode::NOT_FOUND), false);
    } else if (res.GetResponseCode() != HttpResponseCode::NOT_MODIFIED) {
      // It seem sdk is not support well on If-Modified-Since
      // So, here we do a double check
      auto lastModified = res.GetLastModified();
      time_t mtime =
          lastModified.empty() ? 0 : RFC822GMTToSeconds(lastModified);
      if (mtime > modifiedSince && modified != nullptr) {
        *modified = true;
      }
      auto fileMetaData =
          QSClientUtils::HeadObjectOutputToFileMetaData(path, res);
      if (fileMetaData) {
        auto &dirTree = Drive::Instance().GetDirectoryTree();
        if (dirTree) {
          dirTree->Grow(std::move(fileMetaData));  // add Node to dir tree
        }
      }
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
    //QingStorService::initService(clientConfig.GetClientLogFile());  // TODO(jim):
    QingStorService::initService("/home/jim/qsfs.test/qsfs.log/");

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
