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

#include "qingstor/Utils.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include <exception>
#include <string>
#include <vector>

#include "base/LogMacros.h"

namespace QS {

namespace QingStor {

namespace Utils {

using std::string;
using std::to_string;
using std::vector;

// --------------------------------------------------------------------------
string GetUserName(uid_t uid) {
  int32_t maxBufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
  assert(maxBufSize > 0);
  if (!(maxBufSize > 0)) {
    DebugError("Fail to get maximum size of getpwuid_r() data buffer.");
    return string();
  }

  vector<char> buffer(maxBufSize);
  struct passwd pwdInfo;
  struct passwd *result = NULL;
  if (getpwuid_r(uid, &pwdInfo, &buffer[0], maxBufSize, &result) != 0) {
    DebugError(string("Fail to get passwd information : ") + strerror(errno) +
               ".");
    return string();
  }

  if (result == NULL) {
    DebugInfo("No data in passwd of " + to_string(uid) + ".");
    return string();
  }

  auto userName = result->pw_name;
  DebugInfoIf(userName == NULL,
              "Empty username of uid " + to_string(uid) + ".");
  return userName != NULL ? userName : string();
}

// --------------------------------------------------------------------------
bool IsIncludedInGroup(uid_t uid, gid_t gid) {
  int32_t maxBufSize = sysconf(_SC_GETGR_R_SIZE_MAX);
  assert(maxBufSize > 0);
  if (!(maxBufSize > 0)) {
    DebugError("Fail to get maximum size of getgrgid_r() data buffer.");
    return false;
  }

  vector<char> buffer(maxBufSize);
  struct group grpInfo;
  struct group *result = NULL;
  if (getgrgid_r(gid, &grpInfo, &buffer[0], maxBufSize, &result) != 0) {
    DebugError(string("Fail to get group information : ") + strerror(errno) +
               ".");
    return false;
  }
  if (result == NULL) {
    DebugInfo("No gid in group of " + to_string(gid) + ".");
    return false;
  }

  string userName = GetUserName(uid);
  if (userName.empty()) {
    return false;
  }

  char **groupMember = result->gr_mem;
  while (groupMember && *groupMember) {
    if (userName == *groupMember) {
      return true;
    }
    ++groupMember;
  }

  return false;
}

// --------------------------------------------------------------------------
bool GetProcessEffectiveUserID(uid_t *uid) {
  try {
    *uid = geteuid();
  } catch (const std::exception &err) {
    DebugError(err.what());
    return false;
  } catch (...) {
    DebugError(
        "Error while getting the effective user ID of the calling process.");
    return false;
  }
  return true;
}

// --------------------------------------------------------------------------
bool GetProcessEffectiveGroupID(gid_t *gid) {
  try {
    *gid = getegid();
  } catch (const std::exception &err) {
    DebugError(err.what());
    return false;
  } catch (...) {
    DebugError(
        "Error while getting the effective group ID of the calling process.");
    return false;
  }
  return true;
}

// --------------------------------------------------------------------------
bool HavePermission(struct stat *st) {
  uid_t uidProcess = -1;
  if (!GetProcessEffectiveUserID(&uidProcess)) {
    return false;
  }
  gid_t gidProcess = -1;
  if (!GetProcessEffectiveGroupID(&gidProcess)) {
    return false;
  }

  DebugInfo("[Process: uid=" + to_string(uidProcess) + ", gid=" +
            to_string(gidProcess) + "] - [File uid=" + to_string(st->st_uid) +
            ", gid=" + to_string(st->st_gid) + "]");

  // Check owner
  if (0 == uidProcess || st->st_uid == uidProcess) {
    return true;
  }

  // Check group
  if (st->st_gid == gidProcess || IsIncludedInGroup(uidProcess, st->st_gid)) {
    if (S_IRWXG == (st->st_mode & S_IRWXG)) {
      return true;
    }
  }

  // Check others
  if (S_IRWXO == (st->st_mode & S_IRWXO)) {
    return true;
  }

  return false;
}

// --------------------------------------------------------------------------
bool HavePermission(const std::string &path) {
  struct stat st;
  int errorCode = stat(path.c_str(), &st);
  if (errorCode != 0) {
    DebugError("Unable to access " + path +
               " when trying to check its permission.");
    return false;
  } else {
    return HavePermission(&st);
  }
}

}  // namespace Utils
}  // namespace QingStor
}  // namespace QS
