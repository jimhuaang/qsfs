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

#include "qingstor-sdk-cpp/HttpCommon.h"

#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "filesystem/Configure.h"
#include "filesystem/MimeTypes.h"

namespace QS {

namespace Client {

namespace QSClientUtils {

using QingStor::GetBucketStatisticsOutput;
using QingStor::HeadObjectOutput;
using QingStor::Http::HttpResponseCode;
using QingStor::ListObjectsOutput;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::FileSystem::Configure::GetBlockSize;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Configure::GetDefineDirMode;
using QS::FileSystem::Configure::GetFragmentSize;
using QS::FileSystem::Configure::GetNameMaxLen;
using QS::FileSystem::GetDirectoryMimeType;
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
  if (output.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
    return nullptr;
  }

  auto size = static_cast<uint64_t>(output.GetContentLength());
  // TODO(jim): mode should do with meta when skd support this
  mode_t mode = size == 0 ? GetDefineDirMode() : GetDefineFileMode();
  // TODO(jim): type definition may need to consider list object
  // bool isDir = output.GetContentType() == GetDirectoryMimeType();
  bool isDir = size == 0;
  FileType type = isDir ? FileType::Directory : FileType::File;
  // TODO(jim) : sdk bug no last modified when obj key is dir
  time_t mtime = output.GetLastModified().empty() ? 
                    time(NULL) : RFC822GMTToSeconds(output.GetLastModified());
  bool encrypted = !output.GetXQSEncryptionCustomerAlgorithm().empty();
  return make_shared<FileMetaData>(
      objKey, size, time(NULL), mtime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, type, output.GetContentType(),
      output.GetETag(), encrypted);
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> ObjectKeyToFileMetaData(const KeyType &objectKey,
                                                 const string &dirPath,
                                                 time_t atime) {
  // Do const cast as sdk does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  return make_shared<FileMetaData>(
      dirPath + key.GetKey(),  // build full path
      static_cast<uint64_t>(key.GetSize()), atime,
      static_cast<time_t>(key.GetModified()), GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), GetDefineFileMode(), FileType::File,
      key.GetMimeType(), key.GetEtag(), key.GetEncrypted());
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> CommonPrefixToFileMetaData(const string &commonPrefix,
                                                    const string &dirPath,
                                                    time_t atime) {
  auto fullPath = AppendPathDelim(dirPath + commonPrefix);
  return make_shared<FileMetaData>(
      fullPath, 0, atime, atime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), GetDefineDirMode(),
      FileType::Directory);  // TODO(jim): sdk api (meta)
}

// --------------------------------------------------------------------------
vector<shared_ptr<FileMetaData>> ListObjectsOutputToFileMetaDatas(
    const ListObjectsOutput &listObjsOutput, bool addSelf) {
  // Do const cast as sdk does not provide const-qualified accessors
  ListObjectsOutput &output = const_cast<ListObjectsOutput &>(listObjsOutput);
  vector<shared_ptr<FileMetaData>> metas;
  if (output.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
    return metas;
  }

  time_t atime = time(NULL);
  auto dirPath = AppendPathDelim("/" + output.GetPrefix());
  // Add dir itself
  if (addSelf) {
    metas.push_back(make_shared<FileMetaData>(
        dirPath, 0, atime, atime, GetProcessEffectiveUserID(),
        GetProcessEffectiveGroupID(), GetDefineDirMode(), FileType::Directory));
  }
  // Add files
  for (const auto &key : output.GetKeys()) {
    metas.push_back(std::move(ObjectKeyToFileMetaData(key, dirPath, atime)));
  }
  // Add subdirs
  for (const auto &commonPrefix : output.GetCommonPrefixes()) {
    metas.push_back(
        std::move(CommonPrefixToFileMetaData(commonPrefix, dirPath, atime)));
  }
  return metas;
}

}  // namespace QSClientUtils
}  // namespace Client
}  // namespace QS
