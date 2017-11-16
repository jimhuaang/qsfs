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

#ifndef INCLUDE_CLIENT_CLIENTFACTORY_H_
#define INCLUDE_CLIENT_CLIENTFACTORY_H_

#include <memory>

namespace QS {

namespace FileSystem {
class Drive;
}  // namespace FileSystem

namespace Client {

class Client;
class ClientImpl;

class ClientFactory {
 public:
  ClientFactory(ClientFactory &&) = delete;
  ClientFactory(const ClientFactory &) = delete;
  ClientFactory &operator=(ClientFactory &&) = delete;
  ClientFactory &operator=(const ClientFactory &) = delete;
  ~ClientFactory() = default;

 public:
  static ClientFactory &Instance();

 private:
  ClientFactory() = default;

  std::shared_ptr<Client> MakeClient();
  std::shared_ptr<ClientImpl> MakeClientImpl();
  friend class Client;
  friend class QS::FileSystem::Drive;
};

}  // namespace Client
}  // namespace QS


#endif  // INCLUDE_CLIENT_CLIENTFACTORY_H_
