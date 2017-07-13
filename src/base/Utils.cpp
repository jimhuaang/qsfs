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
#include <sys/stat.h>
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

std::pair<bool, string> DeleteFilesInDirectoryNoLog(const std::string &path) {
  // TODO(jim) :: refer to s3fs_utils::...
  return {true, ""};
}

bool DeleteFilesInDirectory(const std::string &path) {
  auto outcome = DeleteFilesInDirectoryNoLog(path);
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

void AddDirectorySeperator(string &path) {
  assert(!path.empty());
  DebugWarningIf(path.empty(),
                 "Try to add directory seperator with a empty input.");
  if (path.back() != PATH_DELIM) {
    path.append(1, PATH_DELIM);
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
        str.erase(--str.end());
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
