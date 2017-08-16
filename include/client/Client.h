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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_CLIENT_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_CLIENT_H_  // NOLINT

#include <time.h>

#include <chrono>
#include <condition_variable>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <string>

#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"
#include "client/ClientError.h"
#include "client/ClientFactory.h"
#include "client/QSError.h"
#include "client/RetryStrategy.h"

namespace QS {

namespace FileSystem {
class Drive;
}  // namespace FileSystem

namespace Client {

class ClientImpl;
class RetryStrategy;
class TransferHandle;

class Client {
 public:
  Client(std::shared_ptr<ClientImpl> impl =
             ClientFactory::Instance().MakeClientImpl(),
         std::unique_ptr<QS::Threading::ThreadPool> executor =
             std::unique_ptr<QS::Threading::ThreadPool>(
                 new QS::Threading::ThreadPool(
                     ClientConfiguration::Instance().GetPoolSize())),
         RetryStrategy retryStratety = GetCustomRetryStrategy());

  Client(Client &&) = default;
  Client(const Client &) = delete;
  Client &operator=(Client &&) = default;
  Client &operator=(const Client &) = delete;

  virtual ~Client();

 public:
  virtual bool Connect() = 0;
  virtual bool DisConnect() = 0;

  virtual ClientError<QSError> DeleteFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> DeleteDirectory(const std::string &dirPath) = 0;
  virtual ClientError<QSError> MakeFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> MakeDirectory(const std::string &dirPath) = 0;
  virtual ClientError<QSError> RenameFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> RenameDirectory(const std::string &dirPath) = 0;

  virtual ClientError<QSError> DownloadFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> DownloadDirectory(const std::string &dirPath) = 0;
  virtual ClientError<QSError> UploadFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> UploadDirectory(const std::string &dirPath) = 0;

  virtual ClientError<QSError> ReadFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> ListDirectory(const std::string &dirPath) = 0;
  virtual ClientError<QSError> WriteFile(const std::string &filePath) = 0;
  virtual ClientError<QSError> WriteDirectory(const std::string &dirPath) = 0;

  virtual ClientError<QSError> Stat(const std::string &path,
                                    time_t modifiedSince = 0,
                                    bool *modified = nullptr) = 0;

 public:
  void RetryRequestSleep(std::chrono::milliseconds sleepTime) const;

  const RetryStrategy &GetRetryStrategy() const { return m_retryStrategy; }
  const std::shared_ptr<ClientImpl> &GetClientImpl() const { return m_impl; }

 protected:
  std::shared_ptr<ClientImpl> GetClientImpl() { return m_impl; }
  std::unique_ptr<QS::Threading::ThreadPool> &GetExecutor() {
    return m_executor;
  }

 private:
  std::shared_ptr<ClientImpl> m_impl;
  std::unique_ptr<QS::Threading::ThreadPool> m_executor;
  RetryStrategy m_retryStrategy;
  mutable std::mutex m_retryLock;
  mutable std::condition_variable m_retrySignal;

  friend class QS::FileSystem::Drive;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_CLIENT_H_
