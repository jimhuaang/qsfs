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

#include "base/Utils.h"

#include <assert.h>
#include <errno.h>
#include <string.h>  // for strerror

#include <dirent.h>  // for opendir readdir
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>  // for access

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Utils {

using std::cerr;
using std::pair;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
static const char PATH_DELIM = '/';

namespace {
string PostErrMsg(const string &path) { return path + " : " + strerror(errno); }
}  // namespace

// --------------------------------------------------------------------------
bool CreateDirectoryIfNotExistsNoLog(const string &path) {
  int errorCode =
      mkdir(path.c_str(), QS::FileSystem::Configure::GetDefineDirMode());
  bool success = (errorCode == 0 || errno == EEXIST);
  if (!success) cerr << "Fail to creating directory " + PostErrMsg(path) + "\n";
  return success;
}

// --------------------------------------------------------------------------
bool CreateDirectoryIfNotExists(const string &path) {
  Info("Creating directory " + path);
  bool success = CreateDirectoryIfNotExistsNoLog(path);

  DebugErrorIf(!success, "Fail to creating directory " + PostErrMsg(path));
  return success;
}

// --------------------------------------------------------------------------
bool RemoveDirectoryIfExistsNoLog(const string &path) {
  int errorCode = rmdir(path.c_str());
  bool success = (errorCode == 0 || errno == ENOENT || errno == ENOTDIR);
  if (!success)
    cerr << "Fail to deleteing directory " + PostErrMsg(path) + "\n";
  return success;
}

// --------------------------------------------------------------------------
bool RemoveDirectoryIfExists(const string &path) {
  Info("Deleting directory " + path);
  bool success = RemoveDirectoryIfExistsNoLog(path);

  DebugErrorIf(!success, "Fail to deleting directory " + PostErrMsg(path));
  return success;
}

// --------------------------------------------------------------------------
bool RemoveFileIfExistsNoLog(const string &path) {
  int errorCode = unlink(path.c_str());
  bool success = (errorCode == 0 || errno == ENOENT);
  if (!success) cerr << "Fail to deleting file " + PostErrMsg(path) + "\n";
  return success;
}

// --------------------------------------------------------------------------
bool RemoveFileIfExists(const string &path) {
  Info("Creating file " + path);
  bool success = RemoveFileIfExistsNoLog(path);
  DebugErrorIf(!success, "Fail to deleting file " + PostErrMsg(path));
  return success;
}

// --------------------------------------------------------------------------
pair<bool, string> DeleteFilesInDirectoryNoLog(const std::string &path,
                                               bool deleteSelf) {
  bool success = true;
  string msg;
  auto ErrorOut = [&success, &msg](string &&str) {
    success = false;
    msg.assign(str);
  };

  unique_ptr<DIR, decltype(closedir) *> dir(opendir(path.c_str()), closedir);
  if (dir == nullptr) {
    struct dirent *nextDir = nullptr;
    while ((nextDir = readdir(dir.get())) != nullptr) {
      if (strcmp(nextDir->d_name, ".") == 0 ||
          strcmp(nextDir->d_name, "..") == 0) {
        continue;
      }

      string fullPath(path);
      fullPath.append(1, PATH_DELIM);
      fullPath.append(nextDir->d_name);

      struct stat st;
      if (lstat(fullPath.c_str(), &st) != 0) {
        ErrorOut("Could not get stats of file " + PostErrMsg(fullPath));
        break;
      }

      if (S_ISDIR(st.st_mode)) {
        if (!DeleteFilesInDirectoryNoLog(fullPath, true).first) {
          ErrorOut("Could not remove subdirectory " + PostErrMsg(fullPath));
          break;
        }
      } else {
        if (unlink(fullPath.c_str()) != 0) {
          ErrorOut("Could not remove file " + PostErrMsg(fullPath));
          break;
        }
      }  // end of S_ISDIR
    }    // end of while
  } else {
    ErrorOut("Could not open directory " + PostErrMsg(path));
  }

  if (deleteSelf && rmdir(path.c_str()) != 0) {
    ErrorOut("Could not remove dir " + PostErrMsg(path));
  }

  if (!success) cerr << msg << "\n";

  return {success, msg};
}

// --------------------------------------------------------------------------
bool DeleteFilesInDirectory(const std::string &path, bool deleteSelf) {
  auto outcome = DeleteFilesInDirectoryNoLog(path, deleteSelf);
  DebugErrorIf(!outcome.first, outcome.second);
  return outcome.first;
}

// --------------------------------------------------------------------------
bool FileExists(const string &path) {
  int errorCode = access(path.c_str(), F_OK);
  if (errorCode == 0) {
    return true;
  } else {
    DebugInfo("File " + path + " not exists : " + strerror(errno));
    return false;
  }
}

// --------------------------------------------------------------------------
bool IsDirectory(const string &path) {
  struct stat stBuf;
  if (stat(path.c_str(), &stBuf) != 0) {
    DebugWarning("Unable to access path " + PostErrMsg(path));
    return false;
  } else {
    return S_ISDIR(stBuf.st_mode);
  }
}

// --------------------------------------------------------------------------
bool IsRootDirectory(const std::string &path) { return path == "/"; }

// --------------------------------------------------------------------------
string AddDirectorySeperator(const string &path) {
  assert(!path.empty());
  DebugWarningIf(path.empty(),
                 "Try to add directory seperator with a empty input");
  string copy(path);
  if (path.back() != PATH_DELIM) {
    copy.append(1, PATH_DELIM);
  }
  return copy;
}

// --------------------------------------------------------------------------
string GetPathDelimiter() { return string(1, PATH_DELIM); }

// --------------------------------------------------------------------------
pair<bool, string> GetParentDirectory(const string &path) {
  bool success = false;
  string str;
  if (FileExists(path)) {
    if (IsRootDirectory(path)) {
      success = true;
      str.assign("/");  // return root
    } else {
      str = path;
      if (str.back() == PATH_DELIM) {
        str.pop_back();
      }
      auto pos = str.find_last_of(PATH_DELIM);
      if (pos != string::npos) {
        success = true;
        str = str.substr(0, pos + 1);  // including the ending "/"
      } else {
        str.assign("Unable to find parent director for path " + path);
      }
    }
  } else {
    str.assign("Unable to access path " + path);
  }

  return {success, str};
}

// --------------------------------------------------------------------------
bool IsDirectoryEmpty(const std::string &path) {
  unique_ptr<DIR, decltype(closedir) *> dir(opendir(path.c_str()), closedir);
  if (dir) {
    struct dirent *nextDir = nullptr;
    while ((nextDir = readdir(dir.get())) != nullptr) {
      if (strcmp(nextDir->d_name, ".") != 0 &&
          strcmp(nextDir->d_name, "..") != 0) {
        return false;
      }
    }
  } else {
    Error("Failed to open path " + path + " : " + strerror(errno));
    return false;
  }

  return true;
}

// --------------------------------------------------------------------------
string GetUserName(uid_t uid, bool logOn) {
  int32_t maxBufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
  assert(maxBufSize > 0);
  if (!(maxBufSize > 0)) {
    if (logOn) {
      DebugError("Fail to get maximum size of getpwuid_r() data buffer");
    }
    return string();
  }

  vector<char> buffer(maxBufSize);
  struct passwd pwdInfo;
  struct passwd *result = NULL;
  if (getpwuid_r(uid, &pwdInfo, &buffer[0], maxBufSize, &result) != 0) {
    if (logOn) {
      DebugError(string("Fail to get passwd information : ") + strerror(errno));
    }
    return string();
  }

  if (result == NULL) {
    if (logOn) {
      DebugInfo("No data in passwd of " + to_string(uid));
    }
    return string();
  }

  string userName(result->pw_name);
  if (logOn) {
    DebugInfoIf(userName.empty(), "Empty username of uid " + to_string(uid));
  }
  return userName;
}

// --------------------------------------------------------------------------
bool IsIncludedInGroup(uid_t uid, gid_t gid, bool logOn) {
  int32_t maxBufSize = sysconf(_SC_GETGR_R_SIZE_MAX);
  assert(maxBufSize > 0);
  if (!(maxBufSize > 0)) {
    if (logOn) {
      DebugError("Fail to get maximum size of getgrgid_r() data buffer");
    }
    return false;
  }

  vector<char> buffer(maxBufSize);
  struct group grpInfo;
  struct group *result = NULL;
  if (getgrgid_r(gid, &grpInfo, &buffer[0], maxBufSize, &result) != 0) {
    if (logOn) {
      DebugError(string("Fail to get group information : ") + strerror(errno));
    }
    return false;
  }
  if (result == NULL) {
    if (logOn) {
      DebugInfo("No gid in group of " + to_string(gid));
    }
    return false;
  }

  string userName = GetUserName(uid, logOn);
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
uid_t GetProcessEffectiveUserID() {
  static uid_t uid = geteuid();
  return uid;
}

// --------------------------------------------------------------------------
gid_t GetProcessEffectiveGroupID() {
  static gid_t gid = getegid();
  return gid;
}

// --------------------------------------------------------------------------
bool HavePermission(struct stat *st, bool logOn) {
  uid_t uidProcess = GetProcessEffectiveUserID();
  gid_t gidProcess = GetProcessEffectiveGroupID();

  if (logOn) {
    DebugInfo("[Process: uid=" + to_string(uidProcess) + ", gid=" +
              to_string(gidProcess) + "] - [File uid=" + to_string(st->st_uid) +
              ", gid=" + to_string(st->st_gid) + "]");
  }

  // Check owner
  if (0 == uidProcess || st->st_uid == uidProcess) {
    return true;
  }

  // Check group
  if (st->st_gid == gidProcess ||
      IsIncludedInGroup(uidProcess, st->st_gid, logOn)) {
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
bool HavePermission(const std::string &path, bool logOn) {
  struct stat st;
  int errorCode = stat(path.c_str(), &st);
  if (errorCode != 0) {
    if (logOn) {
      DebugError("Unable to access " + path +
                 " when trying to check its permission");
    }
    return false;
  } else {
    if (logOn) {
      DebugInfo("Check file permission of " + path);
    }
    return HavePermission(&st, logOn);
  }
}

}  // namespace Utils
}  // namespace QS
