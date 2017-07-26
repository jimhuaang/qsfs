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

#include "data/ResourceManager.h"

#include <assert.h>

#include <utility>

#include "base/LogMacros.h"

namespace QS {

namespace Data {

using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::vector;

bool ResourceManager::ResourcesAvailable() {
  lock_guard<mutex> lock(m_queueLock);
  return !m_resources.empty() && !m_shutdown.load();
}

void ResourceManager::PutResource(Resource resource) {
  m_resources.emplace_back(std::move(resource));
}

Resource ResourceManager::Acquire() {
  unique_lock<mutex> lock(m_queueLock);
  m_semaphore.wait(
      lock, [this] { return m_shutdown.load() || !m_resources.empty(); });

  // Should not go here
  assert(!m_shutdown.load());
  DebugErrorIf(m_shutdown.load(),
               "Tring to acquire resouce BUT resouce manager is shutdown");

  Resource resource = std::move(m_resources.back());
  m_resources.pop_back();

  return resource;
}

void ResourceManager::Release(Resource resource) {
  unique_lock<mutex> lock(m_queueLock);
  m_resources.emplace_back(std::move(resource));
  lock.unlock();
  m_semaphore.notify_one();
}

vector<Resource> ResourceManager::ShutdownAndWait(size_t resourceCount) {
  unique_lock<mutex> lock(m_queueLock);
  m_shutdown.store(true);
  m_semaphore.wait(lock, [this, resourceCount] { return m_resources.size() >= resourceCount; });
  auto resources = std::move(m_resources);
  m_resources.clear();
  return resources;
}

}  // namespace Data
}  // namespace QS
