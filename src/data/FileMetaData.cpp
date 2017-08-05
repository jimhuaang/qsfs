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

#include "data/FileMetaData.h"

#include "base/LogMacros.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Data {

using std::string;

FileMetaData::FileMetaData(const string &fileName, uint64_t fileSize,
                           time_t atime, time_t mtime, uid_t uid, gid_t gid,
                           mode_t fileMode, FileType fileType,
                           const string &mimeType, dev_t dev)
    : m_fileName(fileName),
      m_fileSize(fileSize),
      m_atime(atime),
      m_mtime(mtime),
      m_ctime(mtime),
      m_uid(uid),
      m_gid(gid),
      m_fileMode(fileMode),
      m_fileType(fileType),
      m_mimeType(mimeType),
      m_dev(dev),
      m_eTag(std::string()),
      m_encrypted(false),
      m_dirty(false),
      m_write(false),
      m_fileOpen(false),
      m_pendingGet(false),
      m_pendingCreate(false) {
  m_numLink = fileType == FileType::Directory ? 2 : 1;
  m_cachedTime = time(NULL);
}

struct stat FileMetaData::ToStat() const {
  struct stat st;
  st.st_size = m_fileSize;
  if (m_fileType == FileType::File) {
    st.st_blocks = QS::FileSystem::Configure::GetBlocks(st.st_size);
  }
  st.st_blksize = QS::FileSystem::Configure::GetBlockSize();
  st.st_atim = {m_atime, 0};
  st.st_mtim = {m_mtime, 0};
  st.st_ctim = {m_ctime, 0};
  st.st_uid = m_uid;
  st.st_gid = m_gid;
  st.st_mode = GetFileTypeAndMode();
  st.st_dev = m_dev;
  // this may need to always set with 1, to see fuse FAQ. TODO(jim):
  st.st_nlink = m_numLink;

  return st;
}

mode_t FileMetaData::GetFileTypeAndMode() const {
  mode_t stmode;
  switch (m_fileType) {
    case FileType::File:
      stmode = S_IFREG | m_fileMode;
      break;
    case FileType::Directory:
      stmode = S_IFDIR | m_fileMode;
      break;
    case FileType::SymLink:
      stmode = S_IFLNK | m_fileMode;
      break;
    case FileType::Block:
      stmode = S_IFBLK | m_fileMode;
      break;
    case FileType::Character:
      stmode = S_IFCHR | m_fileMode;
      break;
    case FileType::FIFO:
      stmode = S_IFIFO | m_fileMode;
      break;
    case FileType::Socket:
      stmode = S_IFSOCK | m_fileMode;
      break;
    default:
      stmode = S_IFREG | m_fileMode;
      break;
  }
  return stmode;
}

}  // namespace Data
}  // namespace QS
