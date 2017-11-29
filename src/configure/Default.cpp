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

#include "configure/Default.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "data/Size.h"

namespace QS {

namespace Configure {

namespace Default {

using std::string;
using std::vector;

static const char* const PROGRAM_NAME = "qsfs";
static const char* const QSFS_DEFAULT_CREDENTIALS = "/opt/qsfs/qsfs.cred";
static const char* const QSFS_DEFAULT_DISK_CACHE_DIR = "/tmp/qsfs_cache/";
static uint16_t const    QSFS_DEFAULT_MAX_RETRIES = 3;
static const char* const QSFS_DEFAULT_LOG_DIR = "/opt/qsfs/qsfs_log/";
static const char* const QSFS_DEFAULT_LOGLEVEL_NAME = "INFO";
static const char* const QSFS_DEFAULT_HOST = "qingstor.com";
static const char* const QSFS_DEFAULT_PROTOCOL = "https";
static const char *const QSFS_DEFAULT_ZONE = "pek3a";
static const char* const MIME_FILE_DEBIAN = "/etc/mime.types";
static const char* const MIME_FILE_CENTOS = "/usr/share/mime/types";


const char* GetProgramName() { return PROGRAM_NAME; }

string GetDefaultCredentialsFile() { return QSFS_DEFAULT_CREDENTIALS; }
string GetDefaultDiskCacheDirectory() { return QSFS_DEFAULT_DISK_CACHE_DIR; }
uint16_t GetDefaultMaxRetries() { return QSFS_DEFAULT_MAX_RETRIES; }
string GetDefaultLogDirectory() { return QSFS_DEFAULT_LOG_DIR; }
string GetDefaultLogLevelName() { return QSFS_DEFAULT_LOGLEVEL_NAME; }
string GetDefaultHostName() { return QSFS_DEFAULT_HOST; }

uint16_t GetDefaultPort(const string &protocolName) {
  static const uint16_t HTTP_DEFAULT_PORT = 80;
  static const uint16_t HTTPS_DEFAULT_PORT = 443;

  string lowercaseProtocol(protocolName);
  for (auto &c : lowercaseProtocol) {
    c = std::tolower(c);
  }

  if (lowercaseProtocol == "http") {
    return HTTP_DEFAULT_PORT;
  } else if (lowercaseProtocol == "https") {
    return HTTPS_DEFAULT_PORT;
  } else {
    return HTTPS_DEFAULT_PORT;
  }
}

string GetDefaultProtocolName() { return QSFS_DEFAULT_PROTOCOL; }
string GetDefaultZone() { return QSFS_DEFAULT_ZONE; }

vector<string> GetMimeFiles() {
  vector<string> mimes = { MIME_FILE_DEBIAN, MIME_FILE_CENTOS };
  return mimes;
}

uint16_t GetPathMaxLen() { return 4096; }  // TODO(jim): should be 1023?
uint16_t GetNameMaxLen() { return 255; }

mode_t GetRootMode() { return (S_IRWXU | S_IRWXG | S_IRWXO); }
mode_t GetDefineFileMode() { return (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); }
mode_t GetDefineDirMode() {
  return (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

// 4096 byte is the default allocation block for ext2/ext3/ext4 filesystem
uint16_t GetBlockSize() { return 4096; }
uint16_t GetFragmentSize() { return 4096; }
blkcnt_t GetBlocks(off_t size) {
  // A directory reserves one block for meta-data
  return size / 512 + 1;
}

uint64_t GetMaxCacheSize() {
  return QS::Data::Size::MB100;  // default value
}

size_t GetMaxStatCount() {
  return QS::Data::Size::K20;  // default value
}

uint16_t GetMaxListObjectsCount() {
  return QS::Data::Size::K1;  // default value
}

static const int CLIENT_DEFAULT_POOL_SIZE = 5;
static const int QS_CONNECTION_DEFAULT_RETRIES = 3;  // qs sdk parameter
static const char* QS_SDK_LOG_FILE_NAME = "qingstor_sdk_log.txt";  // qs sdk log

int GetClientDefaultPoolSize() { return CLIENT_DEFAULT_POOL_SIZE; }

uint32_t GetTransactionDefaultTimeDuration() {
  return 500;  // in milliseconds
}

int GetQSConnectionDefaultRetries() { return QS_CONNECTION_DEFAULT_RETRIES; }

const char* GetQingStorSDKLogFileName() { return QS_SDK_LOG_FILE_NAME; }

size_t GetDefaultParallelTransfers() { return 5; }

uint64_t GetDefaultTransferMaxBufHeapSize() { return QS::Data::Size::MB50; }

uint64_t GetDefaultTransferBufSize() {
  // should be larger than 2 * MB4 (min part size) = 8MB,
  // as QSTransferManager count on it to average the last two
  // multiparts size when do multipart upload
  return QS::Data::Size::MB10;
}

uint64_t GetUploadMultipartMinPartSize() {
  // qs qingstor sepcific
  return QS::Data::Size::MB4;
}

uint64_t GetUploadMultipartMaxPartSize() { return QS::Data::Size::GB1; }

uint64_t GetUploadMultipartThresholdSize() { return QS::Data::Size::MB20; }

}  // namespace Default
}  // namespace Configure
}  // namespace QS
