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

#ifndef INCLUDE_CLIENT_CLIENTIMPL_H_
#define INCLUDE_CLIENT_CLIENTIMPL_H_

#include <memory>

#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"

namespace QS {

namespace Client {

class ClientImpl {
 public:
  ClientImpl(std::unique_ptr<QS::Threading::ThreadPool> executor =
                 std::unique_ptr<QS::Threading::ThreadPool>(
                     new QS::Threading::ThreadPool(
                         ClientConfiguration::Instance().GetPoolSize())));
  ClientImpl(ClientImpl &&) = default;
  ClientImpl(const ClientImpl &) = default;
  ClientImpl &operator=(ClientImpl &&) = default;
  ClientImpl &operator=(const ClientImpl &) = default;
  virtual ~ClientImpl();

 protected:
  const std::unique_ptr<QS::Threading::ThreadPool> &GetExecutor() const {
    return m_executor;
  }

 private:
  std::unique_ptr<QS::Threading::ThreadPool> m_executor;
};

}  // namespace Client
}  // namespace QS


#endif  // INCLUDE_CLIENT_CLIENTIMPL_H_
