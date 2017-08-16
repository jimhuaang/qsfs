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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_  // NOLINT

#include <memory>
#include <string>

#include "client/Client.h"

namespace QingStor {
class QingStorService;
}  // namespace QingStor

namespace QS {

namespace Client {

class QSClientImpl;

class QSClient : public Client {
 public:
  QSClient();
  QSClient(QSClient &&) = default;
  QSClient(const QSClient &) = delete;
  QSClient &operator=(QSClient &&) = default;
  QSClient &operator=(const QSClient &) = delete;
  ~QSClient();

 public:
  bool Connect() override;
  bool DisConnect() override;

  ClientError<QSError> DeleteFile(const std::string &filePath) override;
  ClientError<QSError> DeleteDirectory(const std::string &dirPath) override;
  ClientError<QSError> MakeFile(const std::string &filePath) override;
  ClientError<QSError> MakeDirectory(const std::string &dirPath) override;
  ClientError<QSError> RenameFile(const std::string &filePath) override;
  ClientError<QSError> RenameDirectory(const std::string &dirPath) override;

  ClientError<QSError> DownloadFile(const std::string &filePath) override;
  ClientError<QSError> DownloadDirectory(const std::string &dirPath) override;
  ClientError<QSError> UploadFile(const std::string &filePath) override;
  ClientError<QSError> UploadDirectory(const std::string &dirPath) override;

  ClientError<QSError> ReadFile(const std::string &filePath) override;
  // Notice the dirPath should end with delimiter
  ClientError<QSError> ListDirectory(const std::string &dirPath) override;
  ClientError<QSError> WriteFile(const std::string &filePath) override;
  ClientError<QSError> WriteDirectory(const std::string &dirPath) override;

  // Get object meta data
  // Using modifiedSince to match if the object modified since then.
  // Using modifiedSince = 0 to always get object meta data, this is default.
  // Using modified to gain output of object modified status since given time.
  // Notes: Stat will grow the directory asynchronizely if objcet is directory
  // and if object is modified comparing with the moment before this operation.
  ClientError<QSError> Stat(const std::string &path, time_t modifiedSince = 0,
                            bool *modified = nullptr ) override;

  // Get information about mounted bucket.
  ClientError<QSError> Statvfs(struct statvfs *stvfs) override;

 public:
  static const std::unique_ptr<QingStor::QingStorService> &GetQingStorService();
  const std::shared_ptr<QSClientImpl> &GetQSClientImpl() const;

 private:
  std::shared_ptr<QSClientImpl> &GetQSClientImpl();
  static void StartQSService();
  void CloseQSService();
  void InitializeClientImpl();

 private:
  static std::unique_ptr<QingStor::QingStorService> m_qingStorService;
  std::shared_ptr<QSClientImpl> m_qsClientImpl;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCLIENT_H_
