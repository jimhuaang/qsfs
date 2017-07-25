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

#include "filesystem/Initializer.h"

#include <memory>
#include <mutex>  // NOLINT

namespace QS {

namespace FileSystem {

using std::unique_ptr;

static std::once_flag initOnceFlag;
static bool deferInitializer = true;
unique_ptr<Initializer::InitFunctionQueue> Initializer::m_queue = nullptr;

Initializer::Initializer(const PriorityInitFuncPair &initFuncPair) {
  SetInitializer(initFuncPair);
}

void Initializer::RunInitializers() {
  while (!m_queue->empty()) {
    (m_queue->top().second)();
    m_queue->pop();
  }
  deferInitializer = false;
}

void Initializer::RemoveInitializers() {
  while (!m_queue->empty()) {
    m_queue->pop();
  }
  deferInitializer = false;
}

void Initializer::SetInitializer(const PriorityInitFuncPair &pair) {
  if (deferInitializer) {
    std::call_once(initOnceFlag, [] {
      m_queue = std::unique_ptr<InitFunctionQueue>(new InitFunctionQueue);
    });
    m_queue->emplace(pair);
  } else {
    (pair.second)();
  }
}

}  // namespace FileSystem
}  // namespace QS
