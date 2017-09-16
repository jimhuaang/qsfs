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

#include "filesystem/Mounter.h"

#include <errno.h>
#include <string.h>  // for strerror, strcmp

#include <assert.h>
#include <dirent.h>
#include <stdio.h>   // for popen
#include <stdlib.h>  // for system
#include <sys/stat.h>

#include <memory>
#include <mutex>  // NOLINT
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/Utils.h"
#include "filesystem/Drive.h"
#include "filesystem/IncludeFuse.h"  // for fuse.h
#include "filesystem/Operations.h"
#include "filesystem/Options.h"

namespace QS {

namespace FileSystem {

using QS::Exception::QSException;
using std::call_once;
using std::once_flag;
using std::pair;
using std::string;
using std::unique_ptr;

static unique_ptr<Mounter> instance(nullptr);
static once_flag flag;

// --------------------------------------------------------------------------
Mounter &Mounter::Instance() {
  call_once(flag, [] { instance.reset(new Mounter); });
  return *instance.get();
}

// --------------------------------------------------------------------------
// IsMountable only checking following things:
// the mount point is
//   - not root
//   - accessable, means exist
//   - is dir
//   - process has permission to access it
//
// Notes, IsMountable assume currently the mount point is still not mounted,
// otherwise, stat a mounted point always fail.
Mounter::Outcome Mounter::IsMountable(const std::string &mountPoint,
                                      bool logOn) const {
  bool success = true;
  string msg;
  auto ErrorOut = [&success, &msg](string &&str) {
    success = false;
    msg.assign(str);
  };

  if (QS::Utils::IsRootDirectory(mountPoint)) {
    ErrorOut("Unable to mount to root directory");
    return {success, msg};
  }

  struct stat stBuf;
  if (stat(mountPoint.c_str(), &stBuf) != 0) {
    ErrorOut("Unable to access MOUNTPOINT " + mountPoint + " : " +
             strerror(errno));
  } else if (!S_ISDIR(stBuf.st_mode)) {
    ErrorOut("MOUNTPOINT " + mountPoint + " is not a directory");
  } else if (!QS::Utils::HavePermission(mountPoint, logOn)) {
    ErrorOut("MOUNTPOINT " + mountPoint + " permisson denied");
  }

  return {success, msg};
}

// --------------------------------------------------------------------------
bool Mounter::IsMounted(const string &mountPoint, bool logOn) const {
  bool mounted = false;
  string command("cat /etc/mtab | grep " + mountPoint + " | wc -c");
  FILE *pFile = popen(command.c_str(), "r");
  if (pFile != NULL && fgetc(pFile) != '0') {
    mounted = true;
  }
  pclose(pFile);

  return mounted;
}

// --------------------------------------------------------------------------
bool Mounter::Mount(const Options &options, bool logOn) const {
  const auto &drive = QS::FileSystem::Drive::Instance();
  if (!drive.IsMountable()) {
    throw QSException("Unable to mount ...");
  }
  return DoMount(options, logOn, nullptr);
}

// --------------------------------------------------------------------------
void Mounter::UnMount(const string &mountPoint, bool logOn) const {
  if (IsMounted(mountPoint, logOn)) {
    string command("fusermount -u " + mountPoint + " | wc -c");
    FILE *pFile = popen(command.c_str(), "r");
    if (pFile != NULL && fgetc(pFile) != '0') {
      if (logOn) {
        Error("Unable to unmout filesystem at " + mountPoint +
              ". Trying lazy unmount");
      }
      command.assign("fusermount -uqz " + mountPoint);
      system(command.c_str());
    }
    pclose(pFile);
    if (logOn) {
      Info("Unmount qsfs sucessfully");
    }
  } else {
    if (logOn) {
      Warning("Trying to unmount filesystem at " + mountPoint +
              " which is not mounted");
    }
  }
}

// --------------------------------------------------------------------------
bool Mounter::DoMount(const Options &options, bool logOn,
                      void *user_data) const {
  static fuse_operations qsfsOperations;
  InitializeFUSECallbacks(&qsfsOperations);

  // Do really mount
  auto &fuseArgs = const_cast<Options &>(options).GetFuseArgs();
  auto &mountPoint = options.GetMountPoint();
  static int maxTries = 3;
  int count = 0;
  do {
    if (!IsMounted(mountPoint, logOn)) {
      int ret =
          fuse_main(fuseArgs.argc, fuseArgs.argv, &qsfsOperations, user_data);
      if (0 != ret) {
        errno = ret;
        throw QSException("Unable to mount qsfs");
      } else {
        return true;
      }
    } else {
      if (++count > maxTries) {
        if (logOn) {
          Error("Unable to unmount " + mountPoint);
        }
        return false;
      }
      if (logOn) {
        Warning(mountPoint +
                "is already mounted. Tring to unmount, and mount again");
      }
      string command("umount -l " + mountPoint);  // lazy detach filesystem
      system(command.c_str());
    }
  } while (true);

  return false;
}

}  // namespace FileSystem
}  // namespace QS
