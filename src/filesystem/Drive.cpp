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

#include "filesystem/Drive.h"

#include <time.h>

#include <sys/types.h>

#include <memory>

#include "base/Utils.h"
#include "data/Directory.h"
#include "filesystem/Configure.h"

namespace QS {

namespace FileSystem {

using QS::Data::DirectoryTree;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using std::make_shared;

Drive::Drive() : m_mountable(true) {
  uid_t uid = -1;
  if (!GetProcessEffectiveUserID(&uid, true)) {
    m_mountable.store(false);
  }
  gid_t gid = -1;
  if (!GetProcessEffectiveGroupID(&gid, true)) {
    m_mountable.store(false);
  }

  m_directoryTree = make_shared<DirectoryTree>(time(NULL), uid, gid,
                                               Configure::GetRootMode());

  // TODO(jim): init cache and client
}

bool Drive::IsMountable() { return m_mountable.load(); }

}  // namespace FileSystem
}  // namespace QS
