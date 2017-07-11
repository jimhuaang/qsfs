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

#ifndef _QSFS_FUSE_INCLUDE_QINGSTOR_CONFIGURE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_QINGSTOR_CONFIGURE_H_  // NOLINT

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <string>

namespace QS {

namespace QingStor {

namespace Configure {

const uint64_t MB5 = 5 * 1024 * 1024;
const uint64_t MB10 = 10 * 1024 * 1024;
const uint64_t MB20 = 20 * 1024 * 1024;
const uint64_t MB100 = 100 * 1024 * 1024;

std::string GetQSVersion();

std::string GetConfigureDirectory();
std::string GetCredentialsFile();
std::string GetConfigureFile();
std::string GetDefaultLogDirectory();
std::string GetLogDirectory();

uint16_t GetPathMaxLen();
uint16_t GetNameMaxLen();

mode_t GetRootMode();
mode_t GetDefineFileMode();
mode_t GetDefineDirMode();

uint16_t GetBlockSize();
uint16_t GetFragmentSize();

size_t GetMaxCacheSize();

bool IsSafeDiskSpace();

struct Account {
  std::string m_mountPoint;
  std::string m_logPath;
  std::string m_logLevel;
  std::string m_useCache;
};

}  // namespace Configure
}  // namespace QingStor
}  // namespace QS

// NOLINTNEXTLIN
#endif  // _QSFS_FUSE_INCLUDE_QINGSTOR_CONFIGURE_H_
