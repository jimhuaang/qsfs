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

#ifndef THREAD_H_POOL
#define THREAD_H_POOL

#include <stddef.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace QS {

namespace Threading {

class TaskHandle;

using Task = std::unique_ptr<std::function<void()>>;

class ThreadPool {
 public:
  ThreadPool(size_t poolSize);
  ~ThreadPool();

  ThreadPool(ThreadPool &&) = delete;
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

 public:
  void SubmitTask(Task task);

 private:
  //Task &&PopTask();
  Task PopTask();
  bool HasTasks();

 private:
  size_t m_poolSize;
  std::queue<Task> m_tasks;
  std::mutex m_queueLock;
  std::vector<std::unique_ptr<TaskHandle>> m_taskHandles;

  std::mutex m_syncLock;
  std::condition_variable m_syncConditionVar;

  friend class TaskHandle;
};

}  // namespace Threading
}  // namespace QS

#endif  // THREAD_H_POOL
