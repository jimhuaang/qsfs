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

#include "client/ClientFactory.h"

#include <memory>
#include <mutex>  // NOLINT

#include "client/Client.h"
#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
#include "client/NullClient.h"
#include "client/NullClientImpl.h"
#include "client/QSClient.h"
#include "client/QSClientImpl.h"
#include "client/URI.h"

namespace QS {

namespace Client {

using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

static unique_ptr<ClientFactory> instance(nullptr);
static std::once_flag flag;

ClientFactory &ClientFactory::Instance() {
  std::call_once(flag, [] { instance.reset(new ClientFactory); });
  return *instance.get();
}

shared_ptr<Client> ClientFactory::MakeClient() {
  auto client = shared_ptr<Client>(nullptr);
  auto host = ClientConfiguration::Instance().GetHost();
  switch (host) {
    case Http::Host::QingStor: {
      client = make_shared<QSClient>();
      break;
    }
    // Add other cases here
    case Http::Host::Null:  // Bypass
    default: {
      client = make_shared<NullClient>();
      break;
    }
  }
  return client;
}

shared_ptr<ClientImpl> ClientFactory::MakeClientImpl() {
  auto clientImpl = shared_ptr<ClientImpl>(nullptr);
  auto host = ClientConfiguration::Instance().GetHost();
  switch (host) {
    case Http::Host::QingStor: {
      clientImpl = make_shared<QSClientImpl>();
      break;
    }
    // Add other cases here
    case Http::Host::Null:  // Bypass
    default: {
      clientImpl = make_shared<NullClientImpl>();
      break;
    }
  }
  return clientImpl;
}

}  // namespace Client
}  // namespace QS
