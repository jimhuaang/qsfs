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

#include "client/TransferManager.h"

#include <assert.h>

#include <cmath>
#include <memory>
#include <mutex>  // NOLINT
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "base/ThreadPool.h"
#include "client/NullClient.h"
#include "data/ResourceManager.h"
#include "data/Size.h"

namespace QS {

namespace Client {

using QS::Data::Resource;
using QS::Data::ResourceManager;
using QS::Threading::ThreadPool;
using std::make_shared;
using std::unique_ptr;
using std::vector;

std::once_flag initOnceFlag;

// --------------------------------------------------------------------------
TransferManager::TransferManager(const TransferManagerConfigure &config)
    : m_configure(config), m_client(make_shared<NullClient>()) {
  if (GetBufferCount() > 0) {
    m_bufferManager = unique_ptr<ResourceManager>(new ResourceManager);
  }
  if (GetMaxParallelTransfers() > 0) {
    m_executor =
        unique_ptr<ThreadPool>(new ThreadPool(config.m_maxParallelTransfers));
  }
}

// --------------------------------------------------------------------------
TransferManager::~TransferManager() {
  if (!m_bufferManager) {
    return;
  }
  for (auto &resource : m_bufferManager->ShutdownAndWait(GetBufferCount())) {
    if (resource) {
      resource.reset();
    }
  }
}

// --------------------------------------------------------------------------
size_t TransferManager::GetBufferCount() const {
  return GetBufferSize() == 0 ? 0
                              : static_cast<size_t>(std::ceil(
                                    GetBufferMaxHeapSize() / GetBufferSize()));
}

// --------------------------------------------------------------------------
void TransferManager::SetClient(const std::shared_ptr<Client> &client) {
  assert(client);
  if (client) {
    m_client = client;
    std::call_once(initOnceFlag, [this] { InitializeResources(); });
  } else {
    DebugError("Null client parameter");
  }
}

// --------------------------------------------------------------------------
void TransferManager::InitializeResources() {
  if(!m_bufferManager){
    DebugError("Buffer Manager is null");
    return;
  }
  for(uint64_t i = 0; i < GetBufferMaxHeapSize(); i += GetBufferSize()){
    m_bufferManager->PutResource(Resource(new vector<char>(GetBufferSize())));
  }
}

}  // namespace Client
}  // namespace QS
