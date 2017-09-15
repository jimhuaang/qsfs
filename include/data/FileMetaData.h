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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <mutex>  // NOLINT
#include <string>

namespace QS {

namespace Data {

class Entry;
class FileMetaData;

enum class FileType {
  File,
  Directory,
  SymLink,
  Block,
  Character,
  FIFO,
  Socket
};

const std::string &GetFileTypeName(FileType fileType);
// Build a default dir meta
// Set mtime = 0 by default, so as any update based on the condition that if dir
// is modified should still be available.
std::shared_ptr<QS::Data::FileMetaData> BuildDefaultDirectoryMeta(
    const std::string &dirPath, time_t mtime = 0);

/**
 * Object file metadata
 */
class FileMetaData {
 public:
  FileMetaData(const std::string &filePath, uint64_t fileSize, time_t atime,
               time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
               FileType fileType = FileType::File,
               const std::string &mimeType = "",
               const std::string &eTag = std::string(), bool encrypted = false,
               dev_t dev = 0);

  FileMetaData(FileMetaData &&) = default;
  FileMetaData(const FileMetaData &) = default;
  FileMetaData &operator=(FileMetaData &&) = default;
  FileMetaData &operator=(const FileMetaData &) = default;
  ~FileMetaData() = default;

public:
  struct stat ToStat () const;
  mode_t GetFileTypeAndMode() const;
  bool IsDirectory() const { return m_fileType == FileType::Directory; }
  // Return the directory path (ending with "/") this file belongs to
  std::string MyDirName() const;
  std::string MyBaseName() const;
  bool FileAccess(uid_t uid, gid_t gid, int amode) const;

  // accessor
  const std::string &GetFilePath() const { return m_filePath;}

 private:
  FileMetaData() = default;

  // file full path name
  std::string m_filePath;  // For a directory, this will be ending with "/"
  uint64_t m_fileSize;
  // Notice: file creation time is not stored in unix
  time_t m_atime;  // time of last access
  time_t m_mtime;  // time of last modification
  time_t m_ctime;  // time of last file status change
  time_t m_cachedTime;
  uid_t m_uid;        // user ID of owner
  gid_t m_gid;        // group ID of owner
  mode_t m_fileMode;  // file type & mode (permissions)
  FileType m_fileType;
  std::string m_mimeType;
  std::string m_eTag;
  bool m_encrypted = false;
  dev_t m_dev = 0;  // device number (file system)
  int m_numLink = 1;
  bool m_needUpload = false;
  bool m_fileOpen = false;

  friend class Entry;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_DATA_FILEMETADATA_H_
