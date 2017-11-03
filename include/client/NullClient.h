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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_NULLCLIENT_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_NULLCLIENT_H_  // NOLINT

#include <memory>
#include <string>
#include <vector>

#include "client/Client.h"

namespace QS {

namespace Client {

class NullClient : public Client {
 public:
  NullClient()
      : Client(std::shared_ptr<ClientImpl>(nullptr),
               std::unique_ptr<QS::Threading::ThreadPool>(nullptr)) {}
  NullClient(NullClient &&) = default;
  NullClient(const NullClient &) = delete;
  NullClient &operator=(NullClient &&) = default;
  NullClient &operator=(const NullClient &) = delete;
  ~NullClient() = default;

 public:
  ClientError<QSError> HeadBucket(bool useThreadPool) override;

  ClientError<QSError> DeleteFile(const std::string &filePath) override;
  ClientError<QSError> MakeFile(const std::string &filePath) override;
  ClientError<QSError> MakeDirectory(const std::string &dirPath) override;
  ClientError<QSError> MoveFile(const std::string &filePath,
                                const std::string &newFilePath) override;
  ClientError<QSError> MoveDirectory(const std::string &sourceDirPath,
                                     const std::string &targetDirPath,
                                     bool async = false) override;

  ClientError<QSError> DownloadFile(
      const std::string &filePath, const std::shared_ptr<std::iostream> &buffer,
      const std::string &range, std::string *eTag) override;

  ClientError<QSError> InitiateMultipartUpload(const std::string &filePath,
                                               std::string *uploadId) override;

  ClientError<QSError> UploadMultipart(
      const std::string &filePath, const std::string &uploadId, int partNumber,
      uint64_t contentLength,
      const std::shared_ptr<std::iostream> &buffer) override;

  ClientError<QSError> SymLink(const std::string &filePath,
                               const std::string &linkPath) override;

  ClientError<QSError> CompleteMultipartUpload(
      const std::string &filePath, const std::string &uploadId,
      const std::vector<int> &sortedPartIds) override;

  ClientError<QSError> AbortMultipartUpload(
      const std::string &filePath, const std::string &uploadId) override;

  ClientError<QSError> UploadFile(
      const std::string &filePath, uint64_t fileSize,
      const std::shared_ptr<std::iostream> &buffer) override;

  ClientError<QSError> ListDirectory(const std::string &dirPath,
                                     bool useThreadPool) override;

  ClientError<QSError> Stat(const std::string &path, time_t modifiedSince = 0,
                            bool *modified = nullptr) override;
  ClientError<QSError> Statvfs(struct statvfs *stvfs) override;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_NULLCLIENT_H_
