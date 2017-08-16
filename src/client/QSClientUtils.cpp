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

#include "client/QSClientUtils.h"

#include <assert.h>
#include <stdint.h>  // for uint64_t
#include <time.h>

#include <memory>
#include <utility>
#include <vector>

#include <sys/stat.h>  // for mode_t

#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Client {

namespace QSClientUtils {

using QingStor::GetBucketStatisticsOutput;
using QingStor::HeadObjectOutput;
using QingStor::ListObjectsOutput;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::FileSystem::Configure::GetBlockSize;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Configure::GetDefineDirMode;
using QS::FileSystem::Configure::GetFragmentSize;
using QS::FileSystem::Configure::GetNameMaxLen;
using QS::TimeUtils::RFC822GMTToSeconds;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using std::make_shared;
using std::string;
using std::shared_ptr;
using std::vector;

// --------------------------------------------------------------------------
void GetBucketStatisticsOutputToStatvfs(
    const GetBucketStatisticsOutput &bucketStatsOutput, struct statvfs *statv) {
  assert(statv != nullptr);
  if (statv == nullptr) return;

  auto output = const_cast<GetBucketStatisticsOutput &>(bucketStatsOutput);
  uint64_t numObjs = output.GetCount();
  uint64_t bytesUsed = output.GetSize();
  uint64_t freeBlocks = UINT64_MAX;  // object storage is unlimited

  statv->f_bsize = GetBlockSize();      // Filesystem block size
  statv->f_frsize = GetFragmentSize();  // Fragment size
  statv->f_blocks =
      (bytesUsed / statv->f_frsize);    // Size of fs in f_frsize units
  statv->f_bfree = freeBlocks;          // Number of free blocks
  statv->f_bavail = freeBlocks;         // Number of free blocks for unprivileged users
  statv->f_files = numObjs;             // Number of inodes
  statv->f_namemax = GetNameMaxLen();   // Maximum filename length
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> HeadObjectOutputToFileMetaData(
    const string &objKey, const HeadObjectOutput &headObjOutput) {
  auto output = const_cast<HeadObjectOutput &>(headObjOutput);
  // TODO(jim): confirm the mode and type definition
  auto size = static_cast<uint64_t>(output.GetContentLength());
  mode_t mode = size == 0 ? GetDefineDirMode() : GetDefineFileMode();
  FileType type = size == 0 ? FileType::Directory : FileType::File;
  time_t mtime = RFC822GMTToSeconds(output.GetLastModified());
  bool encrypted = !output.GetXQSEncryptionCustomerAlgorithm().empty();
  return make_shared<FileMetaData>(
      objKey, size, time(NULL), mtime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, type, output.GetContentType(),
      output.GetETag(), encrypted);
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> ObjectKeyToFileMetaData(const KeyType &objectKey,
                                                 const string &prefix) {
  // Do const cast as sdk does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  return make_shared<FileMetaData>(
      AppendPathDelim("/" + prefix) + key.GetKey(),  // build full path
      static_cast<uint64_t>(key.GetSize()), time(NULL),
      static_cast<time_t>(key.GetModified()), GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), GetDefineFileMode(), FileType::File,
      key.GetMimeType(), key.GetEtag(), key.GetEncrypted());
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> CommonPrefixToFileMetaData(const string &commonPrefix,
                                                    const string &prefix) {
  time_t atime = time(NULL);
  auto fullPath =
      AppendPathDelim(AppendPathDelim("/" + prefix) + commonPrefix);
  return make_shared<FileMetaData>(
      fullPath, 0, atime, atime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), GetDefineDirMode(),
      FileType::Directory);  // TODO(jim): sdk api (meta)
}

// --------------------------------------------------------------------------
vector<shared_ptr<FileMetaData>> ListObjectsOutputToFileMetaDatas(
    const ListObjectsOutput &listObjsOutput) {
  // Do const cast as sdk does not provide const-qualified accessors
  ListObjectsOutput &output = const_cast<ListObjectsOutput &>(listObjsOutput);
  vector<shared_ptr<FileMetaData>> metas;
  for (const auto &key : output.GetKeys()) {
    metas.push_back(
        std::move(ObjectKeyToFileMetaData(key, output.GetPrefix())));
  }
  for (const auto &commonPrefix : output.GetCommonPrefixes()) {
    metas.push_back(std::move(
        CommonPrefixToFileMetaData(commonPrefix, output.GetPrefix())));
  }
  return metas;
}

}  // namespace QSClientUtils
}  // namespace Client
}  // namespace QS
