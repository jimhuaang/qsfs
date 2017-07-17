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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>  // for access

#include <sstream>
#include <string>

#include "base/LogMacros.h"
#include "qingstor/Configure.h"

namespace QS {

namespace Utils {

using std::string;
static const char PATH_DELIM = '/';
// TODO(jim): refer s3fs_util.cpp

bool CreateDirectoryIfNotExistsNoLog(const string &path) {
  int errorCode =
      mkdir(path.c_str(), QS::QingStor::Configure::GetDefineDirMode());
  return (errorCode == 0 || errno == EEXIST);
}

bool CreateDirectoryIfNotExists(const string &path) {
  Info("Creating directory " + path + ".");
  bool success = CreateDirectoryIfNotExistsNoLog(path);

  DebugErrorIf(
      !success,
      "Fail to creating directory " + path + " : " + strerror(errno) + ".");
  return success;
}

bool RemoveDirectoryIfExistsNoLog(const string &path) {
  int errorCode = rmdir(path.c_str());
  return (errorCode == 0 || errno == ENOENT || errno == ENOTDIR);
}

bool RemoveDirectoryIfExists(const string &path) {
  Info("Deleting directory " + path + ".");
  bool success = RemoveDirectoryIfExistsNoLog(path);

  DebugErrorIf(
      !success,
      "Fail to deleting directory " + path + " : " + strerror(errno) + ".");
  return success;
}

bool RemoveFileIfExistsNoLog(const string &path) {
  int errorCode = unlink(path.c_str());
  return (errorCode == 0 || errno == ENOENT);
}

bool RemoveFileIfExists(const string &path) {
  Info("Creating file " + path + ".");
  bool success = RemoveFileIfExistsNoLog(path);
  DebugErrorIf(!success,
               "Fail to deleting file " + path + " : " + strerror(errno) + ".");
  return success;
}

std::pair<bool, string> DeleteFilesInDirectoryNoLog(const std::string &path,
                                                    bool deleteSelf) {
  bool success = true;
  string msg;
  auto ErrorOut = [&success, &msg](string &&str) {
    success = false;
    msg.assign(str);
  };
  auto PostErrMsg = [](const string &path) -> string {
    return path + " : " + strerror(errno) + ".";
  };

  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    ErrorOut("Could not open directory " + PostErrMsg(path));
  } else {
    struct dirent *nextEntry = nullptr;
    while ((nextEntry = readdir(dir)) != nullptr) {
      if (strcmp(nextEntry->d_name, ".") == 0 ||
          strcmp(nextEntry->d_name, "..") == 0) {
        continue;
      }

      string fullPath(path);
      fullPath.append(1, PATH_DELIM);
      fullPath.append(nextEntry->d_name);

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
  }
  closedir(dir);

  if (deleteSelf && rmdir(path.c_str()) != 0) {
    ErrorOut("Could not remove dir " + PostErrMsg(path));
  }

  return {success, msg};
}

bool DeleteFilesInDirectory(const std::string &path, bool deleteSelf) {
  auto outcome = DeleteFilesInDirectoryNoLog(path, deleteSelf);
  DebugErrorIf(!outcome.first, outcome.second);
  return outcome.first;
}

bool FileExists(const string &path) {
  int errorCode = access(path.c_str(), F_OK);
  if (errorCode == 0) {
    return true;
  } else {
    DebugInfo("File " + path + " not exists : " + strerror(errno) + ".");
    return false;
  }
}

bool IsDirectory(const string &path) {
  struct stat stBuf;
  if (stat(path.c_str(), &stBuf) != 0) {
    DebugWarning("Unable to access path " + path + " : " + strerror(errno) +
                 ".");
    return false;
  } else {
    return S_ISDIR(stBuf.st_mode);
  }
}

bool IsRootDirectory(const std::string &path) { return path == "/"; }

void AddDirectorySeperator(string *path) {
  assert(!path->empty());
  DebugWarningIf(path->empty(),
                 "Try to add directory seperator with a empty input.");
  if (path->back() != PATH_DELIM) {
    path->append(1, PATH_DELIM);
  }
}

std::pair<bool, string> GetParentDirectory(const string &path) {
  bool success = false;
  string str;
  if (FileExists(path)) {
    if (IsRootDirectory(path)) {
      str.assign("Unable to get parent dir for root directory.");
    } else {
      str = path;
      if (str.back() == PATH_DELIM) {
        str.pop_back();
      } else {
        success = true;
        auto pos = str.find_last_of(PATH_DELIM);
        str = str.substr(0, pos);
      }
    }
  } else {
    str.assign("Unable to access path " + path);
  }

  return {success, str};
}

}  // namespace Utils
}  // namespace QS
