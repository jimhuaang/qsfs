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

#include "client/Client.h"

#include <memory>
#include <mutex>  // NOLINT

#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"
#include "client/ClientFactory.h"
#include "client/ClientImpl.h"

namespace QS {

namespace Client {

using QS::Threading::ThreadPool;
using std::make_shared;
using std::shared_ptr;

Client::Client()
    : m_impl(ClientFactory::Instance().MakeClientImpl()),
      m_executor(make_shared<ThreadPool>(
          ClientConfiguration::Instance().GetPoolSize())),
      m_retryStrategy(GetCustomRetryStrategy()) {}

shared_ptr<ClientImpl> Client::GetClientImpl() { return m_impl; }
shared_ptr<ThreadPool> Client::GetExecutor() { return m_executor; }
const RetryStrategy &Client::GetRetryStrategy() const {
  return m_retryStrategy;
}
}  // namespace Client
}  // namespace QS
