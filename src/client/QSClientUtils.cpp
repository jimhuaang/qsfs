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

FileMetaData ObjectKeyToFileMetaData(const KeyType &objectKey) {
  FileMetaData meta;
  // Do const cast as skd does not provide const-qualified accessors
  KeyType &key = const_cast<KeyType &>(objectKey);
  meta.m_fileId = key.GetKey();
  meta.m_fileSize = static_cast<uint64_t>(key.GetSize());
  meta.m_atime = meta.m_cachedTime = time(NULL);
  meta.m_mtime = meta.m_ctime = static_cast<time_t>(key.GetModified());
  meta.m_uid = GetProcessEffectiveUserID();
  meta.m_gid = GetProcessEffectiveGroupID();
  meta.m_fileMode = GetDefineFileMode();
  meta.m_fileType = FileType::File;
  meta.m_mimeType = key.GetMimeType();
  meta.m_numLink = 1;
  meta.m_eTag = key.GetEtag();
  meta.m_encrypted = key.GetEncrypted();
  return meta;
}

FileMetaData CommonPrefixToFileMetaData(const string &commonPrefix) {
  time_t atime = time(NULL);
  return FileMetaData(AddDirectorySeperator(commonPrefix), 0, atime, atime,
                      GetProcessEffectiveUserID(), GetProcessEffectiveGroupID(),
                      GetDefineDirMode(), FileType::Directory);
}

vector<FileMetaData> ListObjectsOutputToFileMetaDatas(const ListObjectsOutput &listObjsOutput){
  // Do const cast as skd does not provide const-qualified accessors
  ListObjectsOutput &output = const_cast<ListObjectsOutput&>(listObjsOutput);
  vector<FileMetaData> metas;
  for(const auto& key : output.GetKeys()){
    metas.push_back(std::move(ObjectKeyToFileMetaData(key)));
  }
  for(const auto& prefix : output.GetCommonPrefixes()){
    metas.push_back(std::move(CommonPrefixToFileMetaData(prefix)));
  }
  return metas;
}

}  // namespace QSClientUtils
}  // namespace Client
}  // namespace QS
