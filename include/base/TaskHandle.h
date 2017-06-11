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

#ifndef THREADTASK_H_INCLUDED
#define THREADTASK_H_INCLUDED

#include <atomic>
#include <thread>

namespace QS {

namespace Threading {

class ThreadPool;

class TaskHandle {
 public:
  TaskHandle(ThreadPool &threadPool);
  ~TaskHandle();

  TaskHandle(TaskHandle &&) = delete;
  TaskHandle(const TaskHandle &) = delete;
  TaskHandle &operator=(TaskHandle &&) = delete;
  TaskHandle &operator=(const TaskHandle &) = delete;

 public:
  void Stop();

 private:
  void operator()();

 private:
  std::atomic<bool> m_continue;
  ThreadPool &m_threadPool;
  std::thread m_thread;
};

}  // namespace Threading
}  // namespace QS

#endif  // THREADTASK_H_INCLUDED
