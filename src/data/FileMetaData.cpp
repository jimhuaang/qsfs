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

#include <memory>
#include <string>
#include <unordered_map>

#include "base/HashUtils.h"
#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Data {

using QS::FileSystem::Configure::GetDefineDirMode;
using QS::HashUtils::EnumHash;
using QS::StringUtils::AccessMaskToString;
using QS::StringUtils::ModeToString;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using QS::Utils::IsRootDirectory;
using std::make_shared;
using std::string;
using std::to_string;
using std::unordered_map;

// --------------------------------------------------------------------------
const string &GetFileTypeName(FileType fileType) {
  static unordered_map<FileType, string, EnumHash> fileTypeNames = {
      {FileType::File, "File"},
      {FileType::Directory, "Directory"},
      {FileType::SymLink, "Symbolic Link"},
      {FileType::Block, "Block"},
      {FileType::Character, "Character"},
      {FileType::FIFO, "FIFO"},
      {FileType::Socket, "Socket"}};
  return fileTypeNames[fileType];
}

// --------------------------------------------------------------------------
std::shared_ptr<FileMetaData> BuildDefaultDirectoryMeta(const string &dirPath) {
  time_t ttime = time(NULL);
  mode_t mode = GetDefineDirMode();
  return make_shared<FileMetaData>(
      AppendPathDelim(dirPath), 0, ttime, ttime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, FileType::Directory);
}

// --------------------------------------------------------------------------
FileMetaData::FileMetaData(const string &filePath, uint64_t fileSize,
                           time_t atime, time_t mtime, uid_t uid, gid_t gid,
                           mode_t fileMode, FileType fileType,
                           const string &mimeType, const string &eTag,
                           bool encrypted, dev_t dev)
    : m_filePath(filePath),
      m_fileSize(fileSize),
      m_atime(atime),
      m_mtime(mtime),
      m_ctime(mtime),
      m_cachedTime(atime),
      m_uid(uid),
      m_gid(gid),
      m_fileMode(fileMode),
      m_fileType(fileType),
      m_mimeType(mimeType),
      m_eTag(eTag),
      m_encrypted(encrypted),
      m_dev(dev),
      m_needUpload(false),
      m_fileOpen(false) {
  m_numLink = fileType == FileType::Directory ? 2 : 1;
  if (fileType == FileType::Directory) {
    m_filePath = AppendPathDelim(m_filePath);
  }
}

// --------------------------------------------------------------------------
struct stat FileMetaData::ToStat() const {
  struct stat st;
  st.st_size = m_fileSize;
  st.st_blocks = QS::FileSystem::Configure::GetBlocks(st.st_size);
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

// --------------------------------------------------------------------------
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

// --------------------------------------------------------------------------
string FileMetaData::MyDirName() const {
  return QS::Utils::GetDirName(m_filePath);
}

// --------------------------------------------------------------------------
string FileMetaData::MyBaseName() const {
  return QS::Utils::GetBaseName(m_filePath);
}

// --------------------------------------------------------------------------
bool FileMetaData::FileAccess(uid_t uid, gid_t gid, int amode) const {
  DebugInfo("Check access permission of " + m_filePath +
            " [Parameter: uid=" + to_string(uid) + ", gid=" + to_string(gid) +
            ", amode=" + AccessMaskToString(amode) +
            "] - [File uid=" + to_string(m_uid) + ", gid=" + to_string(m_gid) +
            ", mode=" + ModeToString(m_fileMode) + "]");

  if (m_filePath.empty()) {
    DebugWarning("object file path is empty");
    return false;
  }

  // Check file existence
  if (amode & F_OK) {
    return true;  // there is a file, always allowed
  }

  bool ret = false;
  // Check read permission
  if (amode & R_OK) {
    if ((uid == m_uid || uid == 0) && (m_fileMode & S_IRUSR)) {
      ret = true;
    } else if ((gid == m_gid || gid == 0) && (m_fileMode & S_IRGRP)) {
      ret = true;
    } else if (m_fileMode & S_IROTH) {
      ret = true;
    } else {
      return false;
    }
  }
  // Check write permission
  if (amode & W_OK) {
    if ((uid == m_uid || uid == 0) && (m_fileMode & S_IWUSR)) {
      ret = true;
    } else if ((gid == m_gid || gid == 0) && (m_fileMode & S_IWGRP)) {
      ret = true;
    } else if (m_fileMode & S_IWOTH) {
      ret = true;
    } else {
      return false;
    }
  }
  // Check execute permission
  if (amode & X_OK) {
    if (uid == 0) {
      // if execute permission is allowed for any user,
      // root shall get execute permission too.
      if ((m_fileMode & S_IXUSR) || (m_fileMode & S_IXGRP) ||
          (m_fileMode & S_IXOTH)) {
        ret = true;
      } else {
        return false;
      }
    } else {
      if ((uid == m_uid) && (m_fileMode & S_IXUSR)) {
        ret = true;
      } else if ((gid == m_gid) && (m_fileMode & S_IXGRP)) {
        ret = true;
      } else if (m_fileMode & S_IXOTH) {
        ret = true;
      } else {
        return false;
      }
    }
  }

  return ret;
}

}  // namespace Data
}  // namespace QS
