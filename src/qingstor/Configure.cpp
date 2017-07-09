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

#include "qingstor/Configure.h"

#include <sys/types.h>
#include <sys/stat.h>

namespace QS {

namespace QingStor {

namespace Configure {

using std::string;

const char* const DEFAULT_DEST_DIR = "/opt/qsfs/";
const char* const QSFS_AUTH_FILE = "qsfs.auth";
const char* const QSFS_CONF_FILE = "qsfs.conf";
const char* const QSFS_LOG_DIR = "qsfs.log/";  // log dir

string GetQSVersion() { return "1.0.0"; }

string GetDestDirectory() { return DEFAULT_DEST_DIR; }

string GetCredentialsFile() {
  // TODO(Jim) : check and return credentials file
  return "";
}

string GetConfigureFile() { return ""; }

string GetLogDirectory() { return ""; }

uint16_t GetPathMaxLen() { return 4096; }
uint16_t GetNameMaxLen() { return 255; }

mode_t GetRootMode() { return (S_IRWXU | S_IRWXG | S_IRWXO); }
mode_t GetDefineFileMode() { return (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); }
mode_t GetDefineDirMode() {
  return (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

uint16_t GetBlockSize() { return 4096; }
uint16_t GetFragmentSize() { return 4096; }

size_t GetMaxCacheSize() {
  // TODO(Jim) : read form config file
  return MB100;
}

}  // namespace Configure
}  // namespace QingStor
}  // namespace QS
