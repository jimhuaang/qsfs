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


#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <utility>


namespace QS {

namespace Utils {

// Create directory if it doesn't exists
//
// @param  : dir path
// @return : bool
bool CreateDirectoryIfNotExistsNoLog(const std::string &path);
bool CreateDirectoryIfNotExists(const std::string &path);

// Remove directory if it exists
//
// @param  : dir path
// @return : bool
bool RemoveDirectoryIfExistsNoLog(const std::string &path);
bool RemoveDirectoryIfExists(const std::string &path);

// Remove file if it exists
//
// @param  : file path
// @return : bool
bool RemoveFileIfExistsNoLog(const std::string &path);
bool RemoveFileIfExists(const std::string &path);

// Delete files in dir recursively
//
// @param  : dir path, flag to delete dir itself
// @return : a pair of {true,""} or {false, message}
//
// This will not print log
std::pair<bool, std::string> DeleteFilesInDirectoryNoLog(
    const std::string &path, bool deleteDirectorySelf);

// Delete files in dir recursively
//
// @param  : dir path, flag to delete dir itself
// @return : bool
//
// Print error message if fail to delete
bool DeleteFilesInDirectory(const std::string &path, bool deleteDirectorySelf);

// Check if file exists
bool FileExists(const std::string &path);

// Check if file is a directory
bool IsDirectory(const std::string &path);

// Check if path is root
bool IsRootDirectory(const std::string &path);

// Append delim to path
//
// @param  : file path
// @return : path appended
std::string AppendPathDelim(const std::string &path);

// Get path delimiter
std::string GetPathDelimiter();

// Get dir name where the file belongs to
//
// @param  : file path
// @return : dir name ending with "/"
//
// If path is root or cannot find dir, return null string
std::string GetDirName(const std::string &path);

// Get file name from file path
//
// @param  : file path
// @return : file name
//
// If path is root or cannot find base name, return null string
std::string GetBaseName(const std::string &path);

// Get parent dir of file
//
// @param  : file path
// @return : a pair of {true, parent dir} or {false, message}
//
// At beginning will check if the file exists, if not return false.
// If file exists, return the file dir name.
std::pair<bool, std::string> GetParentDirectory(const std::string &path);

// Check if dir is empty
//
// @param  : dir path
// @return : bool
bool IsDirectoryEmpty(const std::string &dir);

// Get user name of uid
//
// @param  : uid, log on flag
// @return : user name
std::string GetUserName(uid_t uid, bool logOn);

// Check if given uid is included in group of gid
//
// @param  : uid, gid, log on flag
// @return : bool
bool IsIncludedInGroup(uid_t uid, gid_t gid, bool logOn);

// Get calling process effective user id
//
// @param  : void
// @return : uid
uid_t GetProcessEffectiveUserID();

// Get calling process effective group id
//
// @param  : void
// @return : gid
gid_t GetProcessEffectiveGroupID();

// Check if process has access permission to the file
//
// @param  : file stat, log on flag
// @return : bool
bool HavePermission(struct stat *st, bool logOn);
bool HavePermission(const std::string &path, bool logOn);

// Convert access mode to string
//
// @param  : access mode
// @return : string of a combination of {R_OK, W_OK, X_OK}, e.g. "R_OK&W_OK"
std::string AccessMaskToString(int amode);

// Convert file mode to string
//
// @param  : file mode
// @return : string in form of [rwxXst]
std::string ModeToString(mode_t mode);

}  // namespace Utils
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_UTILS_H_
