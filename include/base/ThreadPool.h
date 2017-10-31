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

#ifndef _QSFS_FUSE_INCLUDE_BASE_THREADPOOL_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_THREADPOOL_H_  // NOLINT

#include <stddef.h>

#include <condition_variable>  // NOLINT
#include <functional>
#include <future>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"  // FRIEND_TEST

namespace QS {

namespace Threading {

class TaskHandle;

using Task = std::function<void()>;

class ThreadPool {
 public:
  explicit ThreadPool(size_t poolSize);
  ~ThreadPool();

  ThreadPool(ThreadPool &&) = delete;
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

 public:
  void SubmitToThread(Task &&task, bool prioritized = false);

  template <typename F, typename... Args>
  void Submit(F &&f, Args &&... args);

  template <typename F, typename... Args>
  void SubmitPrioritized(F &&f, Args &&... args);

  template <typename ReceivedHandler, typename F, typename... Args>
  void SubmitAsync(ReceivedHandler &&handler, F &&f, Args &&... args);

  template <typename ReceivedHandler, typename F, typename... Args>
  void SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
                              Args &&... args);

  template <typename ReceivedHandler, typename CallerContext, typename F,
            typename... Args>
  void SubmitAsyncWithContext(ReceivedHandler &&handler,
                              CallerContext &&context, F &&f, Args &&... args);

  template <typename ReceivedHandler, typename CallerContext, typename F,
            typename... Args>
  void SubmitAsyncWithContextPrioritized(ReceivedHandler &&handler,
                                         CallerContext &&context, F &&f,
                                         Args &&... args);

 private:
  Task PopTask();
  bool HasTasks();

  // This is intended for a interrupt test only, do not use this
  // except in destructor. After this has been called once, all tasks
  // will never been handled since then.
  void StopProcessing();
  FRIEND_TEST(ThreadPoolTest, TestInterrupt);

 private:
  size_t m_poolSize;
  std::list<Task> m_tasks;
  std::mutex m_queueLock;
  std::vector<std::unique_ptr<TaskHandle>> m_taskHandles;
  std::mutex m_syncLock;
  std::condition_variable m_syncConditionVar;

  friend class TaskHandle;
};

template <typename F, typename... Args>
void ThreadPool::Submit(F &&f, Args &&... args) {
  return SubmitToThread(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
}

template <typename F, typename... Args>
void ThreadPool::SubmitPrioritized(F &&f, Args &&... args) {
  auto fun = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  return SubmitToThread(fun, true);
}

template <typename ReceivedHandler, typename F, typename... Args>
void ThreadPool::SubmitAsync(ReceivedHandler &&handler, F &&f,
                             Args &&... args) {
  auto task =
      std::bind(std::forward<ReceivedHandler>(handler),
                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
                std::forward<Args>(args)...);
  {
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_tasks.emplace_back([task]() { task(); });
  }
  m_syncConditionVar.notify_one();
}

template <typename ReceivedHandler, typename F, typename... Args>
void ThreadPool::SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
                                        Args &&... args) {
  auto task =
      std::bind(std::forward<ReceivedHandler>(handler),
                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
                std::forward<Args>(args)...);
  {
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_tasks.emplace_front([task]() { task(); });
  }
  m_syncConditionVar.notify_one();
}

template <typename ReceivedHandler, typename CallerContext, typename F,
          typename... Args>
void ThreadPool::SubmitAsyncWithContext(ReceivedHandler &&handler,
                                        CallerContext &&context, F &&f,
                                        Args &&... args) {
  auto task =
      std::bind(std::forward<ReceivedHandler>(handler),
                std::forward<CallerContext>(context),
                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
                std::forward<Args>(args)...);
  {
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_tasks.emplace_back([task]() { task(); });
  }
  m_syncConditionVar.notify_one();
}

template <typename ReceivedHandler, typename CallerContext, typename F,
          typename... Args>
void ThreadPool::SubmitAsyncWithContextPrioritized(ReceivedHandler &&handler,
                                                   CallerContext &&context,
                                                   F &&f, Args &&... args) {
  auto task =
      std::bind(std::forward<ReceivedHandler>(handler),
                std::forward<CallerContext>(context),
                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
                std::forward<Args>(args)...);
  {
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_tasks.emplace_front([task]() { task(); });
  }
  m_syncConditionVar.notify_one();
}

}  // namespace Threading
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_THREADPOOL_H_
