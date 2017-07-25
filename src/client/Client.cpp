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

#include "client/ClientFactory.h"
#include "client/ClientImpl.h"
#include "client/NullClientImpl.h"

namespace QS {

namespace Client {

using std::make_shared;
using std::shared_ptr;

static std::once_flag clientImplOnceFlag;

Client::Client(const ClientConfiguration &config)
    : m_impl(make_shared<NullClientImpl>()), m_configuration(config) {}

shared_ptr<ClientImpl> Client::GetClientImpl() {
  std::call_once(clientImplOnceFlag, [this] {
    this->m_impl = ClientFactory::Instance().MakeClientImpl();
  });
  return m_impl;
}

}  // namespace Client
}  // namespace QS
