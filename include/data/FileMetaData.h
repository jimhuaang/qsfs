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

#ifndef _QSFS_FUSE_INCLUDED_DATA_FILEMETADATA_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_FILEMETADATA_H_  // NOLINT

#include <stdint.h>
#include <time.h>

#include <unistd.h>

#include <string>

namespace QS {

namespace Data {

enum class FileType {
  None,
  File,
  Directory,
  SymLink,
  Block,
  Character,
  FIFO,
  Socket
};  // TODO(jim): remove None.


const std::string &GetFileTypeName(FileType fileType);

/**
 * Object file metadata
 */
struct FileMetaData {
 public:
  std::string m_fileId;  // file Id used to identify file in cache
  uint64_t m_fileSize;
  // notice file creation time is not stored in unix
  time_t m_atime;  // time of last access
  time_t m_mtime;  // time of last modification
  time_t m_ctime;  // time of last file status change
  time_t m_cachedTime;
  uid_t m_uid;        // user ID of owner
  gid_t m_gid;        // group ID of owner
  mode_t m_fileMode;  // file type & mode (permissions)
  FileType m_fileType;
  std::string m_mimeType;
  dev_t m_dev = 0;  // device number (file system)
  int m_numLink = 0;
  std::string m_eTag;
  bool m_encrypted = false;
  bool m_dirty = false;
  bool m_write = false;
  bool m_fileOpen = false;
  bool m_pendingGet = false;
  bool m_pendingCreate = false;

 public:
  FileMetaData(const std::string &fileId, uint64_t fileSize, time_t atime,
               time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
               FileType fileType = FileType::None, std::string mimeType = "",
               dev_t dev = 0)
      : m_fileId(fileId),
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
    // TODO(jim): Consider other types.
    m_numLink = fileType == FileType::Directory
                    ? 2
                    : fileType == FileType::File ? 1 : 0;
    m_cachedTime = time(NULL);
  }

  FileMetaData() = default;
  FileMetaData(FileMetaData &&) = default;
  FileMetaData(const FileMetaData &) = default;
  FileMetaData &operator=(FileMetaData &&) = default;
  FileMetaData &operator=(const FileMetaData &) = default;
  ~FileMetaData() = default;

  // TODO(jim) :
  // struct stat ToStat();
};

}  // namespace Data
}  // namespace QS


// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_DATA_FILEMETADATA_H_
