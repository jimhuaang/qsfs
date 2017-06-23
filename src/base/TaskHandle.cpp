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

#include "base/TaskHandle.h"

#include <functional>

#include "base/ThreadPool.h"

namespace QS {

namespace Threading {

using std::mutex;
using std::unique_lock;

TaskHandle::TaskHandle(ThreadPool &threadPool)
    : m_continue(true),
      m_threadPool(threadPool),
      m_thread([this]() { (*this)(); }) {}

TaskHandle::~TaskHandle() {
  Stop();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void TaskHandle::Stop() { m_continue = false; }

void TaskHandle::operator()() {
  while (m_continue) {
    /*    while (m_continue && m_threadPool.HasTasks()) {
          auto task = m_threadPool.PopTask();
          if (task) {
            (*task)();
            task.reset();
          }
        }

        unique_lock<mutex> lock(m_threadPool.m_syncLock);
        m_threadPool.m_syncConditionVar.wait(
            lock, [this]() { return !m_continue || m_threadPool.HasTasks(); });
    */
    Task task(nullptr);
    {
      unique_lock<mutex> lock(m_threadPool.m_syncLock);
      m_threadPool.m_syncConditionVar.wait(
          lock, [this]() { return !m_continue || m_threadPool.HasTasks(); });
      if (!m_continue) break;
      if (m_threadPool.HasTasks()) {
        // task = std::move(m_threadPool.PopTask());
        task = m_threadPool.PopTask();
      }
    }
    if (task) {
      (*task)();
      task.reset();
    }
  }
}

}  // namespace Threading
}  // namespace QS
