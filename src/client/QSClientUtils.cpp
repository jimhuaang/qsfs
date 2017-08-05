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

#include <stdint.h>  // for uint64_t
#include <time.h>

#include <utility>
#include <vector>

#include "base/Utils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Client {

namespace QSClientUtils {

using QingStor::ListObjectsOutput;
using QS::Data::FileMetaData;
using QS::Data::FileType;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Configure::GetDefineDirMode;
using QS::Utils::AddDirectorySeperator;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using std::string;
using std::vector;

FileMetaData ObjectKeyToFileMetaData(const KeyType &objectKey,
                                     const string &prefix) {
  // Do const cast as sdk does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  FileMetaData meta;
  // add root denoter and the prefix to build the full file path
  meta.m_fileName = AddDirectorySeperator("/" + prefix) + key.GetKey();
  meta.m_fileSize = static_cast<uint64_t>(key.GetSize());
  meta.m_atime = meta.m_cachedTime = time(NULL);
  meta.m_mtime = meta.m_ctime = static_cast<time_t>(key.GetModified());
  meta.m_uid = GetProcessEffectiveUserID();   // TODO(jim): sdk api (meta)
  meta.m_gid = GetProcessEffectiveGroupID();  // TODO(jim): sdk api (meta)
  meta.m_fileMode = GetDefineFileMode();      // TODO(jim): sdk api (meta)
  meta.m_fileType = FileType::File;
  meta.m_mimeType = key.GetMimeType();
  meta.m_numLink = 1;
  meta.m_eTag = key.GetEtag();
  meta.m_encrypted = key.GetEncrypted();
  return meta;
}

FileMetaData CommonPrefixToFileMetaData(const string &commonPrefix,
                                        const string &prefix) {
  time_t atime = time(NULL);
  auto fullPath =
      AddDirectorySeperator(AddDirectorySeperator("/" + prefix) + commonPrefix);
  FileMetaData meta(fullPath, 0, atime, atime, GetProcessEffectiveUserID(),
                      GetProcessEffectiveGroupID(), GetDefineDirMode(),
                      FileType::Directory);  // TODO(jim): sdk api (meta)
  meta.m_cachedTime = atime;
  return meta;
}

vector<FileMetaData> ListObjectsOutputToFileMetaDatas(
    const ListObjectsOutput &listObjsOutput) {
  // Do const cast as sdk does not provide const-qualified accessors
  ListObjectsOutput &output = const_cast<ListObjectsOutput &>(listObjsOutput);
  vector<FileMetaData> metas;
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
