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

#ifndef _QSFS_FUSE_INCLUDED_FILESYSTEM_INITIALIZER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_FILESYSTEM_INITIALIZER_H_  // NOLINT

#include <functional>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

// Declare main in global namespace before class Initializer, since friend
// declarations
// can only introduce names in the surrounding namespace.
extern int main(int argc, char **argv);

namespace QS {

namespace FileSystem {

enum class Priority : unsigned {
  First = 1,
  Second = 2,
  Third = 3,
  Fourth = 4
  // Add others as you want
};

using InitFunction = std::function<void()>;
using PriorityInitFuncPair = std::pair<Priority, InitFunction>;

class Initializer {
 public:
  explicit Initializer(const PriorityInitFuncPair &initFuncPair);

  Initializer(Initializer &&) = delete;
  Initializer(const Initializer &) = delete;
  Initializer &operator=(Initializer &&) = delete;
  Initializer &operator=(const Initializer &) = delete;
  ~Initializer() = default;

 private:
  static void RunInitializers();
  static void RemoveInitializers();
  void SetInitializer(const PriorityInitFuncPair &pair);
  friend int ::main(int argc, char **argv);

 private:
  Initializer() = default;

  struct Greater {
    bool operator()(const PriorityInitFuncPair &left,
                    const PriorityInitFuncPair &right) const {
      return static_cast<unsigned>(left.first) >
             static_cast<unsigned>(right.first);
    }
  };

  using InitFunctionQueue =
      std::priority_queue<PriorityInitFuncPair,
                          std::vector<PriorityInitFuncPair>, Greater>;
  static std::unique_ptr<InitFunctionQueue> m_queue;
};

}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_INITIALIZER_H_
