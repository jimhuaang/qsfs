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

#include "filesystem/Configure.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "data/Size.h"
#include "filesystem/Options.h"

namespace QS {

namespace FileSystem {

namespace Configure {

using std::string;

const char* const PROGRAM_NAME = "qsfs";
const char* const VERSION = "1.0.0";
const char* const QSFS_DEFAULT_CREDENTIALS = "/opt/qsfs/qsfs.cred";
const char* const QSFS_DEFAULT_LOG_DIR = "/opt/qsfs/qsfs.log/";  // log dir

const char* GetProgramName() { return PROGRAM_NAME; }
const char* GetQSFSVersion() { return VERSION; }

string GetDefaultCredentialsFile() { return QSFS_DEFAULT_CREDENTIALS; }
string GetCredentialsFile() {
  auto customCredentials =
      QS::FileSystem::Options::Instance().GetCredentialsFile();
  static const char* credentials = customCredentials.empty()
                                       ? QSFS_DEFAULT_CREDENTIALS
                                       : customCredentials.c_str();
  return credentials;
}

string GetDefaultLogDirectory() { return QSFS_DEFAULT_LOG_DIR; }
string GetLogDirectory() {
  auto customLogDir = QS::FileSystem::Options::Instance().GetLogDirectory();
  static const char* logdir =
      customLogDir.empty() ? QSFS_DEFAULT_LOG_DIR : customLogDir.c_str();
  return logdir;
}

uint16_t GetPathMaxLen() { return 4096; }
uint16_t GetNameMaxLen() { return 255; }

mode_t GetRootMode() { return (S_IRWXU | S_IRWXG | S_IRWXO); }
mode_t GetDefineFileMode() { return (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); }
mode_t GetDefineDirMode() {
  return (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

uint16_t GetBlockSize() { return 4096; }
uint16_t GetFragmentSize() { return 4096; }
blkcnt_t GetBlocks(off_t size) { 
  return size / 512 + 1; 
}

uint64_t GetMaxFileCacheSize() {
  // TODO(Jim) : add option max_cache 
  return QS::Data::Size::MB100;  // default value
}

size_t GetMaxFileMetaDataEntrys() {
  // TODO(jim): add option max_stat_entrys
  return QS::Data::Size::K10;  // default value
}

bool IsSafeDiskSpace() {
  // TODO (jim) :
  return true;
}

}  // namespace Configure
}  // namespace FileSystem
}  // namespace QS
