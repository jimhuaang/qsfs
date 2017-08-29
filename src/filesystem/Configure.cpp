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

static const char* const PROGRAM_NAME = "qsfs";
static const char* const VERSION = "1.0.0";
static const char* const QSFS_DEFAULT_CREDENTIALS = "/opt/qsfs/qsfs.cred";
static const char* const QSFS_DEFAULT_LOG_DIR = "/opt/qsfs/qsfs.log/";  // log dir
static const char* const QSFS_MIME_FILE = "/etc/mime.types";

const char* GetProgramName() { return PROGRAM_NAME; }
const char* GetQSFSVersion() { return VERSION; }

string GetDefaultCredentialsFile() { return QSFS_DEFAULT_CREDENTIALS; }
string GetCredentialsFile() {
  auto customCredentials =
      QS::FileSystem::Options::Instance().GetCredentialsFile();
  return customCredentials.empty()
             ? QSFS_DEFAULT_CREDENTIALS
             : customCredentials;
}

string GetDefaultLogDirectory() { return QSFS_DEFAULT_LOG_DIR; }
string GetLogDirectory() {
  auto customLogDir = QS::FileSystem::Options::Instance().GetLogDirectory();
  return customLogDir.empty() ? QSFS_DEFAULT_LOG_DIR : customLogDir;
}

string GetMimeFile() { return QSFS_MIME_FILE; }

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

size_t GetMaxFileMetaDataCount() {
  // TODO(jim): add option max_stat_entrys
  return QS::Data::Size::K10;  // default value
}

bool IsSafeDiskSpace() {
  // TODO(jim) :
  return true;
}


static const int CLIENT_DEFAULT_POOL_SIZE = 5;
static const int QS_CONNECTION_DEFAULT_RETRIES = 3;  // qs sdk parameter
static const char *QS_LOG_FILE_NAME = "qs_sdk.log";  // qs sdk log

int GetClientDefaultPoolSize(){
  return CLIENT_DEFAULT_POOL_SIZE;
}

int GetQSConnectionDefaultRetries(){
  return QS_CONNECTION_DEFAULT_RETRIES;
}


const char* GetQSLogFileName(){
  return QS_LOG_FILE_NAME;
}


static const size_t DEFAULT_MAX_PARALLEL_TRANSFERS = 10;
static const uint64_t DEFAULT_MAX_BUF_HEAP_SIZE = QS::Data::Size::MB50;
static const uint64_t DEFAULT_MAX_BUF_SIZE = QS::Data::Size::MB5;

size_t GetDefaultMaxParallelTransfers(){
  return DEFAULT_MAX_PARALLEL_TRANSFERS;
}

uint64_t GetDefaultTransferMaxBufHeapSize(){
  return DEFAULT_MAX_BUF_HEAP_SIZE;
}

uint64_t GetDefaultTransferMaxBufSize() {
  return DEFAULT_MAX_BUF_SIZE;
}

}  // namespace Configure
}  // namespace FileSystem
}  // namespace QS
