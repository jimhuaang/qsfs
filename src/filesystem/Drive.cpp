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
#include <mutex>  // NOLINT

#include "base/LogMacros.h"
#include "base/Utils.h"
#include "client/Client.h"
#include "client/ClientFactory.h"
#include "client/TransferManager.h"
#include "client/TransferManagerFactory.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "filesystem/Configure.h"

namespace QS {

namespace FileSystem {

using QS::Client::ClientFactory;
using QS::Client::TransferManagerConfigure;
using QS::Client::TransferManagerFactory;
using QS::Data::Cache;
using QS::Data::DirectoryTree;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using std::make_shared;

static std::unique_ptr<Drive> instance(nullptr);
static std::once_flag flag;

Drive &Drive::Instance() {
  std::call_once(flag, [] { instance.reset(new Drive); });
  return *instance.get();
}

Drive::Drive()
    : m_mountable(true),
      m_client(ClientFactory::Instance().MakeClient()),
      m_transferManager(
          TransferManagerFactory::Create(TransferManagerConfigure())),
      m_cache(make_shared<Cache>()),
      m_directoryTree(make_shared<DirectoryTree>()) {
  uid_t uid = -1;
  if (!GetProcessEffectiveUserID(&uid, true)) {
    m_mountable.store(false);
    Error("Fail to get process user id when constructing Drive");
    return;
  }
  gid_t gid = -1;
  if (!GetProcessEffectiveGroupID(&gid, true)) {
    m_mountable.store(false);
    Error("Fail to get process group id when constructing Drive");
    return;
  }

  m_directoryTree = make_shared<DirectoryTree>(time(NULL), uid, gid,
                                               Configure::GetRootMode());
}

bool Drive::IsMountable() const { return m_mountable.load(); }

}  // namespace FileSystem
}  // namespace QS
