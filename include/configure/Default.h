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

#ifndef _QSFS_FUSE_INCLUDE_CONFIGURE_DEFAULT_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_CONFIGURE_DEFAULT_H_  // NOLINT

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>  // for blkcnt_t

#include <string>

namespace QS {

namespace Configure {

namespace Default {
  
const char* GetProgramName();
const char* GetQSFSVersion();

std::string GetDefaultCredentialsFile();
std::string GetDefaultLogDirectory();
std::string GetMimeFile();
std::string GetCacheTemporaryDirectory();

uint16_t GetPathMaxLen();
uint16_t GetNameMaxLen();

mode_t GetRootMode();
mode_t GetDefineFileMode();
mode_t GetDefineDirMode();

uint16_t GetBlockSize();  // Block size for filesystem I/O
uint16_t GetFragmentSize();
blkcnt_t GetBlocks(off_t size);  // Number of 512B blocks allocated

uint64_t GetMaxCacheSize();  // File cache size in bytes
size_t GetMaxFileMetaDataCount();  // File meta data max count

int GetQSConnectionDefaultRetries();
uint32_t GetTransactionDefaultTimeDuration();  // in milliseconds
int GetClientDefaultPoolSize();
const char* GetQingStorSDKLogFileName();

size_t GetDefaultMaxParallelTransfers();
uint64_t GetDefaultTransferMaxBufHeapSize();
uint64_t GetDefaultTransferMaxBufSize();

uint64_t GetUploadMultipartMinPartSize();
uint64_t GetUploadMultipartMaxPartSize();
uint64_t GetUploadMultipartThresholdSize();

}  // namespace Default
}  // namespace Configure
}  // namespace QS

// NOLINTNEXTLIN
#endif  // _QSFS_FUSE_INCLUDE_CONFIGURE_DEFAULT_H_
