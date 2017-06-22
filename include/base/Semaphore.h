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

#ifndef _QSFS_FUSE_INCLUDE_BASE_SEMAPHORE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_SEMAPHORE_H_  // NOLINT

#include <stddef.h>

#include <condition_variable>  // NOLINT
#include <functional>
#include <mutex>  // NOLINT

namespace QS {

namespace Threading {

class Semaphore {
 public:
  explicit Semaphore(size_t count = 0) : m_count(count) {}

  Semaphore(Semaphore &&) = delete;
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;
  ~Semaphore() = default;

 public:
  void notify_one();
  void notify_all();
  void wait();

 private:
  std::mutex m_mutex;
  std::condition_variable m_conditionVariable;
  size_t m_count;
};

}  // namespace Threading
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_SEMAPHORE_H_
