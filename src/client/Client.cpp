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
#include <utility>

#include "base/ThreadPool.h"
#include "base/ThreadPoolInitializer.h"
#include "client/ClientImpl.h"

namespace QS {

namespace Client {

using QS::Threading::ThreadPool;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

// --------------------------------------------------------------------------
Client::Client(std::shared_ptr<ClientImpl> impl,
               std::unique_ptr<QS::Threading::ThreadPool> executor,
               RetryStrategy retryStratety)
    : m_impl(std::move(impl)),
      m_executor(std::move(executor)),
      m_retryStrategy(std::move(retryStratety)) {
  QS::Threading::ThreadPoolInitializer::Instance().Register(m_executor.get());
}

// --------------------------------------------------------------------------
Client::~Client() {
  // do nothing
}

// --------------------------------------------------------------------------
void Client::RetryRequestSleep(std::chrono::milliseconds sleepTime) const {
  std::unique_lock<std::mutex> lock(m_retryLock);
  m_retrySignal.wait_for(lock, sleepTime);
}

}  // namespace Client
}  // namespace QS
