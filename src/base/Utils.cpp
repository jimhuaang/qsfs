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

#include <sstream>
#include <string>

#include <errno.h>
#include <sys/stat.h>

#include "base/LogMacros.h"

namespace QS {

namespace Utils {

using std::string;
using std::to_string;

bool CreateDirectoryIfNotExistsNoLog(const string &path) {
  int errorCode = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
  return (errorCode == 0 || errno == EEXIST);
}

bool CreateDirectoryIfNotExists(const string &path) {
  Info("Creating directory " + path + ".");
  bool success = CreateDirectoryIfNotExistsNoLog(path);

  DebugErrorIf(!success,
               "Fail to creating directory " + path +
                   " returned code: " + to_string(errno) + ".");
  return success;
}

bool RemoveDirectoryIfExists(const string &path) {
  Info("Deleting directory " + path + ".");
  int errorCode = rmdir(path.c_str());
  if (errorCode == 0 || errno == ENOENT || errno == ENOTDIR) {
    return true;
  } else {
    DebugError("Fail to deleting directory " + path +
               " returned code: " + to_string(errno) + ".");
    return false;
  }
}

bool RemoveFileIfExists(const string &path) {
  Info("Creating file " + path + ".");
  int errorCode = unlink(path.c_str());
  if (errorCode == 0 || errno == ENOENT) {
    return true;
  } else {
    DebugError("Fail to deleting file " + path +
               " returned code: " + to_string(errno) + ".");
    return false;
  }
}

bool FileExists(const string &path) {
  int errorCode = access(path.c_str(), F_OK);
  if (errorCode == 0) {
    return true;
  } else {
    DebugInfo("File " + path +
              " not exists, returned code: " + to_string(errno) + ".");
    return false;
  }
}

}  // namespace Utils
}  // namespace QS
