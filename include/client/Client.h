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

#include <memory>

#include "client/ClientConfiguration.h"
#include "client/RetryStrategy.h"

namespace QS {

namespace Threading {
class ThreadPool;
}  // namespace Threading;

namespace Client {

class ClientImpl;
class RetryStrategy;
class TransferHandle;

class Client {
 public:
  Client();
  Client(Client &&) = default;
  Client(const Client &) = default;
  Client &operator=(Client &&) = default;
  Client &operator=(const Client &) = default;

  virtual ~Client() = default;

 public:
  virtual void Initialize() = 0;
  virtual bool Connect() = 0;
  virtual bool DisConnect() = 0;

 public:
  const RetryStrategy &GetRetryStrategy() const;

 protected:
  std::shared_ptr<ClientImpl> GetClientImpl();
  std::shared_ptr<QS::Threading::ThreadPool> GetExecutor();

 private:
  std::shared_ptr<ClientImpl> m_impl;
  std::shared_ptr<QS::Threading::ThreadPool> m_executor;
  RetryStrategy m_retryStrategy;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_CLIENT_H_
