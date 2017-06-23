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

#include "base/ThreadPool.h"

#include <utility>

#include "base/TaskHandle.h"

namespace QS {

namespace Threading {

using std::lock_guard;
using std::mutex;
using std::unique_ptr;

ThreadPool::ThreadPool(size_t poolSize) : m_poolSize(poolSize) {
  for (size_t i = 0; i < poolSize; ++i) {
    m_taskHandles.emplace_back(new TaskHandle(*this));
  }
}

ThreadPool::~ThreadPool() {
  for (auto &taskHandle : m_taskHandles) {
    taskHandle->Stop();
  }

  m_syncConditionVar.notify_all();
}

void ThreadPool::SubmitTask(Task task) {
  {
    lock_guard<mutex> lock(m_queueLock);
    m_tasks.push(std::move(task));
  }

  m_syncConditionVar.notify_one();
}

Task ThreadPool::PopTask() {
  lock_guard<mutex> lock(m_queueLock);
  if (!m_tasks.empty()) {
    Task &task = m_tasks.front();
    if (task) {
      m_tasks.pop();
      return std::move(task);
    }
  }

  return nullptr;
}

bool ThreadPool::HasTasks() {
  lock_guard<mutex> lock(m_queueLock);
  return !m_tasks.empty();
}

}  // namespace Threading
}  // namespace QS
