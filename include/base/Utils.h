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

#ifndef _QSFS_FUSE_INCLUDE_BASE_UTILS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_UTILS_H_  // NOLINT

#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace QS {

namespace Utils {

struct EnumHash {
  template <typename T>
  int operator()(T enumValue) const {
    return static_cast<int>(enumValue);
  }
};

struct StringHash {
  int operator()(const std::string &strToHash) const {
    if (strToHash.empty()) {
      return 0;
    }

    int hash = 0;
    for (const auto &charValue : strToHash) {
      hash = charValue + 31 * hash;
    }
    return hash;
  }
};

template <typename P>
std::string PointerAddress(P p) {
  if (std::is_pointer<P>::value) {
    int sz = snprintf(NULL, 0, "%p", p);
    std::vector<char> buf(sz + 1);
    snprintf(&buf[0], sz + 1, "%p", p);
    std::string ss(buf.begin(), buf.end());
    return ss;
  }
  return std::string();
}

bool CreateDirectoryIfNotExistsNoLog(const std::string &path);
bool CreateDirectoryIfNotExists(const std::string &path);

bool RemoveDirectoryIfExistsNoLog(const std::string &path);
bool RemoveDirectoryIfExists(const std::string &path);

bool RemoveFileIfExistsNoLog(const std::string &path);
bool RemoveFileIfExists(const std::string &path);

std::pair<bool, std::string> DeleteFilesInDirectoryNoLog(
    const std::string &path, bool deleteDirectorySelf);
bool DeleteFilesInDirectory(const std::string &path, bool deleteDirectorySelf);

bool FileExists(const std::string &path);
bool IsDirectory(const std::string &path);
bool IsRootDirectory(const std::string &path);

std::string AddDirectorySeperator(const std::string &path);
std::string GetPathDelimiter();

// Return true and parent directory if success,
// return false and message if fail.
std::pair<bool, std::string> GetParentDirectory(const std::string &path);

bool IsDirectoryEmpty(const std::string &dir);

std::string GetUserName(uid_t uid, bool logOn);
// Is given uid included in group of gid.
bool IsIncludedInGroup(uid_t uid, gid_t gid, bool logOn);

uid_t GetProcessEffectiveUserID();
gid_t GetProcessEffectiveGroupID();

bool HavePermission(struct stat *st, bool logOn);
bool HavePermission(const std::string &path, bool logOn);

}  // namespace Utils
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_UTILS_H_
