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

#ifndef _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_  // NOLINT

#include <stdint.h>
#include <time.h>

#include <unistd.h>

#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <utility>

namespace QS {

namespace Data {

class Cache;
class File;

class Entry;
class Node;

using FileNameToNodeUnorderedMap =
    std::unordered_map<std::string, std::shared_ptr<Node> >;
using FileNameToWeakNodeUnorderedMap =
    std::unordered_map<std::string, std::weak_ptr<Node> >;

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

  //friend class Entry;
};

class Entry {
 public:
 public:
  Entry(const std::string &fileId, uint64_t fileSize, time_t atime,
        time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
        FileType fileType = FileType::None, std::string mimeType = "",
        dev_t dev = 0)
      : m_metaData(fileId, fileSize, atime, mtime, uid, gid, fileMode, fileType,
                   mimeType, dev) {}

  Entry(FileMetaData fileMetaData) : m_metaData(fileMetaData) {}

  Entry() = default;
  Entry(Entry &&) = default;
  Entry(const Entry &) = default;
  Entry &operator=(Entry &&) = default;
  Entry &operator=(const Entry &) = default;
  ~Entry() = default;

  operator bool() const {
    return !m_metaData.m_fileId.empty() &&
           m_metaData.m_fileType != FileType::None;
  }

  bool IsDirectory() const {
    return m_metaData.m_fileType == FileType::Directory;
  }

  // accessor
  const std::string &GetFileId() const { return m_metaData.m_fileId; }
  uint64_t GetFileSize() const { return m_metaData.m_fileSize; }
  int GetNumLink() const { return m_metaData.m_numLink; }
  FileType GetFileType() const { return m_metaData.m_fileType; }
  time_t GetMTime() const { return m_metaData.m_mtime; }

 private:
  void DecreaseNumLink() { --m_metaData.m_numLink; }
  void IncreaseNumLink() { ++m_metaData.m_numLink; }
  void SetFileSize(uint64_t size) { m_metaData.m_fileSize = size; }

 private:
  FileMetaData m_metaData;  // file meta data

  friend class Node;
  friend class QS::Data::File;
};

/**
 * Representation of a Node in the directory tree.
 */
class Node {
 public:
  Node(const std::string &fileName, std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent)
      : m_fileName(fileName), m_entry(std::move(entry)), m_parent(parent) {
    m_children.clear();
  }

  Node()
      : Node("", std::unique_ptr<Entry>(nullptr),
             std::shared_ptr<Node>(nullptr)) {}

  Node(const std::string &fileName, std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent, const std::string &symbolicLink)
      : Node(fileName, std::move(entry), parent) {
    // must use m_entry instead of entry which is moved to m_entry now
    if (m_entry && m_entry->GetFileSize() <= symbolicLink.size()) {
      m_symbolicLink = std::string(symbolicLink, 0, m_entry->GetFileSize());
    }
  }

  Node(Node &&) = default;
  Node(const Node &) = delete;
  Node &operator=(Node &&) = default;
  Node &operator=(const Node &) = delete;

  ~Node() {
    if (!m_entry) return;

    m_entry->DecreaseNumLink();
    if (m_entry->GetNumLink() == 0 ||
        (m_entry->GetNumLink() <= 1 && m_entry->IsDirectory())) {
      m_entry.reset();
    }
  }

 public:
  virtual operator bool() const {
    return m_entry ? m_entry->operator bool() : false;
  }

 public:
  bool IsEmpty() const { return m_children.empty(); }
  std::shared_ptr<Node> Find(const std::string &fileName) const;
  const FileNameToNodeUnorderedMap &GetChildren() const;

  std::shared_ptr<Node> Insert(const std::shared_ptr<Node> &child);
  void Remove(const std::shared_ptr<Node> &child);
  void RenameChild(const std::string &oldFileName,
                   const std::string &newFileName);

  // accessor
  std::string GetFileName() const { return m_fileName; }
  const std::unique_ptr<Entry> &GetEntry() const { return m_entry; }
  std::weak_ptr<Node> GetParent() const { return m_parent; }
  std::string GetSymbolicLink() const { return m_symbolicLink; }

  std::string GetFileId() const {
    if (!m_entry) return std::string();
    return m_entry->GetFileId();
  }

 private:
  std::unique_ptr<Entry> &GetEntry() { return m_entry; }

 private:
  std::string m_fileName;  // file path
  std::unique_ptr<Entry> m_entry;
  std::weak_ptr<Node> m_parent;
  std::string m_symbolicLink;
  FileNameToNodeUnorderedMap m_children;

  friend class QS::Data::Cache;
};

/**
 * Representation of the filesystem's directory tree.
 */
class DirectoryTree {
 public:
  DirectoryTree(time_t mtime, uid_t uid, gid_t gid, mode_t mode);
  DirectoryTree() = default;
  DirectoryTree(DirectoryTree &&) = delete;
  DirectoryTree(const DirectoryTree &) = delete;
  DirectoryTree &operator=(DirectoryTree &&) = delete;
  DirectoryTree &operator=(const DirectoryTree &) = delete;
  ~DirectoryTree() { m_map.clear(); }

 public:
 private:
  std::shared_ptr<Node> m_root;
  std::shared_ptr<Node> m_currentNode;
  std::recursive_mutex m_mutex;
  FileNameToWeakNodeUnorderedMap m_map;  // record all nodes
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_
