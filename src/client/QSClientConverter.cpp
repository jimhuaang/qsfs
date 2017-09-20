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

#include "client/QSClientConverter.h"

#include <assert.h>
#include <stdint.h>  // for uint64_t
#include <time.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <sys/stat.h>  // for mode_t

#include "qingstor-sdk-cpp/HttpCommon.h"

#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "data/FileMetaData.h"
#include "filesystem/Configure.h"
#include "filesystem/MimeTypes.h"

namespace QS {

namespace Client {

namespace QSClientConverter {

using QingStor::GetBucketStatisticsOutput;
using QingStor::HeadObjectOutput;
using QingStor::Http::HttpResponseCode;
using QingStor::ListObjectsOutput;
using QS::Data::BuildDefaultDirectoryMeta;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::FileSystem::Configure::GetBlockSize;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Configure::GetDefineDirMode;
using QS::FileSystem::Configure::GetFragmentSize;
using QS::FileSystem::Configure::GetNameMaxLen;
using QS::FileSystem::GetDirectoryMimeType;
using QS::FileSystem::GetSymlinkMimeType;
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
  uint64_t bytesTotal = UINT64_MAX; // object storage is unlimited
  uint64_t bytesFree = bytesTotal - bytesUsed;

  statv->f_bsize = GetBlockSize();      // Filesystem block size
  statv->f_frsize = GetFragmentSize();  // Fragment size
  statv->f_blocks =
      (bytesTotal / statv->f_frsize);   // Size of fs in f_frsize units
  statv->f_bfree = 
      (bytesFree / statv->f_frsize);    // Number of free blocks
  statv->f_bavail =
      statv->f_bfree;        // Number of free blocks for unprivileged users
  statv->f_files = numObjs;  // Number of inodes
  statv->f_namemax = GetNameMaxLen();  // Maximum filename length
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> HeadObjectOutputToFileMetaData(
    const string &objKey, const HeadObjectOutput &headObjOutput) {
  auto output = const_cast<HeadObjectOutput &>(headObjOutput);
  if (output.GetResponseCode() == HttpResponseCode::NOT_FOUND) {
    return nullptr;
  }

  auto size = static_cast<uint64_t>(output.GetContentLength());

  // obey mime type for now, may need update in future,
  // as object storage has no dir concept,
  // a dir could have no application/x-directory mime type.
  auto mimeType = output.GetContentType();
  bool isDir = mimeType == GetDirectoryMimeType();
  FileType type = isDir ? FileType::Directory
                        : mimeType == GetSymlinkMimeType() ? FileType::SymLink
                                                           : FileType::File;

  // TODO(jim): mode should do with meta when skd support this
  mode_t mode = isDir ? GetDefineDirMode() : GetDefineFileMode();

  // head object should contain meta such as mtime, but we just do a double
  // check as it can be have no meta data e.g when response code=NOT_MODIFIED
  auto lastModified = output.GetLastModified();
  assert(!lastModified.empty());
  time_t atime = time(NULL);
  time_t mtime = lastModified.empty() ? 0 : RFC822GMTToSeconds(lastModified);
  bool encrypted = !output.GetXQSEncryptionCustomerAlgorithm().empty();
  return make_shared<FileMetaData>(
      objKey, size, atime, mtime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, type, mimeType,
      output.GetETag(), encrypted);
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> ObjectKeyToFileMetaData(const KeyType &objectKey,
                                                 time_t atime) {
  // Do const cast as sdk does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  auto fullPath = "/" + key.GetKey();  // build full path
  auto mimeType = key.GetMimeType();
  bool isDir = mimeType == GetDirectoryMimeType();
  FileType type = isDir ? FileType::Directory
                        : mimeType == GetSymlinkMimeType() ? FileType::SymLink
                                                           : FileType::File;
  // TODO(jim): mode should do with meta when skd support this
  mode_t mode = isDir ? GetDefineDirMode() : GetDefineFileMode();
  return make_shared<FileMetaData>(
      fullPath, static_cast<uint64_t>(key.GetSize()), atime,
      static_cast<time_t>(key.GetModified()), GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, type, mimeType, key.GetEtag(),
      key.GetEncrypted());
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> ObjectKeyToDirMetaData(const KeyType &objectKey,
                                                time_t atime) {
  // Do const cast as sdk does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  auto fullPath = AppendPathDelim("/" + key.GetKey());  // build full path

  return make_shared<FileMetaData>(
      fullPath, 0, atime, static_cast<time_t>(key.GetModified()),
      GetProcessEffectiveUserID(), GetProcessEffectiveGroupID(),
      GetDefineDirMode(), FileType::Directory, GetDirectoryMimeType(),
      key.GetEtag(), key.GetEncrypted());
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> CommonPrefixToFileMetaData(const string &commonPrefix,
                                                    time_t atime) {
  auto fullPath = "/"+ commonPrefix;
  // Walk aroud, as ListObject return no meta for a dir, so set mtime=0.
  // This is ok, as any update based on the condition that if dir is modified
  // should still be available.
  time_t mtime = 0;
  return make_shared<FileMetaData>(
      fullPath, 0, atime, mtime, GetProcessEffectiveUserID(),
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
  string prefix = output.GetPrefix();
  const KeyType *dirItselfAsKey = nullptr;
  // Add files
  for (auto &key : output.GetKeys()) {
    // sdk will put dir itself into keys, ignore it
    if (prefix == key.GetKey()) {
      dirItselfAsKey = &key;
      continue;
    }
    metas.push_back(std::move(ObjectKeyToFileMetaData(key, atime)));
  }
  // Add subdirs
  for (const auto &commonPrefix : output.GetCommonPrefixes()) {
    metas.push_back(std::move(CommonPrefixToFileMetaData(commonPrefix, atime)));
  }
  // Add dir itself
  if (addSelf) {
    auto dirPath = AppendPathDelim("/" + prefix);
    if (std::find_if(metas.begin(), metas.end(),
                  [dirPath](const shared_ptr<FileMetaData> &meta) {
                    return meta->GetFilePath() == dirPath;
                  }) == metas.end()) {
      if(dirItselfAsKey != nullptr){
        metas.push_back(ObjectKeyToDirMetaData(*dirItselfAsKey, atime));
      } else {
        metas.push_back(BuildDefaultDirectoryMeta(dirPath));
      }
    }
  }

  return metas;
}

}  // namespace QSClientConverter
}  // namespace Client
}  // namespace QS
