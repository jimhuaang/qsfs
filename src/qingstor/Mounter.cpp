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

#include "qingstor/Mounter.h"

#include <memory>
#include <mutex>  // NOLINT

#include "base/LogMacros.h"
#include "qingstor/Options.h"

namespace QS {

namespace QingStor {

using std::call_once;
using std::once_flag;
using std::string;
using std::unique_ptr;

static unique_ptr<Mounter> instance(nullptr);
static once_flag flag;

Mounter &Mounter::Instance() {
  call_once(flag, [] { instance.reset(new Mounter); });
  return *instance.get();
}
bool Mounter::IsMountable(const std::string &mountPoint) const {
  // TODO(jim) :
  return true;
}

bool Mounter::IsMounted(const string &mountPoint) const {
  // TODO(jim) : add code
  return true;
}

bool Mounter::Mount(const Options &options) const {
  // TODO(jim)
  return true;
}

bool Mounter::MountLite(const Options &options) const {
  // TODO(jim)
  return true;
}

bool Mounter::UnMount(const string &moutPoint) const {
  // TODO(jim)
  return true;
}

}  // namespace QingStor
}  // namespace QS
