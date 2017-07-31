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

#include "client/TransferManagerFactory.h"

#include <memory>

#include "client/ClientConfiguration.h"
#include "client/NullTransferManager.h"
#include "client/QSTransferManager.h"
#include "client/TransferManager.h"

namespace QS {

namespace Client {

using std::make_shared;
using std::shared_ptr;

shared_ptr<TransferManager> TransferManagerFactory::Create(
    const TransferManagerConfigure &config) {
  auto transferManager = shared_ptr<TransferManager>(nullptr);
  auto host = ClientConfiguration::Instance().GetHost();
  switch (host) {
    case Http::Host::QingStor: {
      transferManager = make_shared<QSTransferManager>(config);
      break;
    }

    // Add other cases here
    case Http::Host::Null:  // Bypass
    default: {
      TransferManagerConfigure nullConfig = {0, 0, 0};
      transferManager = make_shared<NullTransferManager>(nullConfig);
      break;
    }
  }
  return transferManager;
}

}  // namespace Client
}  // namespace QS
