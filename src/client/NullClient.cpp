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

#include "client/NullClient.h"

#include "client/ClientError.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

namespace {
ClientError<QSError> GoodState() {
  return ClientError<QSError>(QSError::GOOD, false);
}
}  // namespace

using std::string;

ClientError<QSError> NullClient::HeadBucket() {
  return GoodState();
}

ClientError<QSError> NullClient::DeleteFile(const string &filePath) {
  return GoodState();
}

ClientError<QSError> NullClient::MakeFile(const std::string &filePath) {
  return GoodState();
}

ClientError<QSError> NullClient::MakeDirectory(const std::string &dirPath) {
  return GoodState();
}

ClientError<QSError> NullClient::MoveFile(const std::string &filePath,
                                          const std::string &newFilePath) {
  return GoodState();
}

ClientError<QSError> NullClient::MoveDirectory(const string &sourceDirPath,
                                               const string &targetDirPath,
                                               bool async) {
  return GoodState();
}

ClientError<QSError> NullClient::DownloadFile(
    const std::string &filePath, const std::shared_ptr<std::iostream> &buffer,
    const std::string &range, std::string *eTag) {
  return GoodState();
}

ClientError<QSError> NullClient::InitiateMultipartUpload(
    const std::string &filePath, std::string *uploadId) {
  return GoodState();
}

ClientError<QSError> NullClient::UploadMultipart(
    const std::string &filePath, const std::string &uploadId, int partNumber,
    uint64_t contentLength, const std::shared_ptr<std::iostream> &buffer) {
  return GoodState();
}

ClientError<QSError> NullClient::CompleteMultipartUpload(
    const std::string &filePath, const std::string &uploadId,
    const std::vector<int> &sortedPartIds) {
  return GoodState();
}

ClientError<QSError> NullClient::AbortMultipartUpload(
    const std::string &filePath, const std::string &uploadId) {
  return GoodState();
}

ClientError<QSError> NullClient::UploadFile(
    const std::string &filePath, uint64_t fileSize,
    const std::shared_ptr<std::iostream> &buffer) {
  return GoodState();
}

ClientError<QSError> NullClient::SymLink(const std::string &filePath,
                                         const std::string &linkPath) {
  return GoodState();
}

ClientError<QSError> NullClient::ListDirectory(const std::string &dirPath) {
  return GoodState();
}

ClientError<QSError> NullClient::Stat(const std::string &path,
                                      time_t modifiedSince, bool *modified) {
  return GoodState();
}

ClientError<QSError> NullClient::Statvfs(struct statvfs *stvfs) {
  return GoodState();
}

}  // namespace Client
}  // namespace QS
