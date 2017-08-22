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

#ifndef _QSFS_FUSE_INCLUDE_BASE_LOGLEVEL_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_LOGLEVEL_H_  // NOLINT

#include <string>

namespace QS {

namespace Logging {

enum class LogLevel : int { Info = 0, Warn = 1, Error = 2, Fatal = 3 };

// Get log level name
//
// @param  : log level enumeration
// @return : log level name
const std::string &GetLogLevelName(LogLevel logLevel);

// Get log level
//
// @param  : log level name
// @return : log level enumeration
//
// Return Info if name not belongs to {INFO, WARN, ERROR, FATAL}
LogLevel GetLogLevelByName(const std::string &name);

// Get log level prefix
//
// @param  : log level enumeration
// @return : prefix
std::string GetLogLevelPrefix(LogLevel logLevel);

}  // namespace Logging
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_LOGLEVEL_H_
