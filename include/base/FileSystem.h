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

#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED

#include <stdint.h>
#include <string.h>
#include <time.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace QS {

namespace FileSystem {

class Entry;
class Node;

using StlFileNameToNodeMap = std::unordered_map<std::string, std::shared_ptr<Node>>;

enum class FileType { None, File, Directory }; // TODO add more types, remove None

/**
 * QingStor object file metadata
 */
class Entry {
 public:
  struct FileMetaData {
    time_t m_atime;  // time of last access
    time_t m_mtime;  // time of last modification
    time_t m_ctime;  // time of last file status change
    time_t m_cachedTime;
    uid_t  m_uid;  // user ID of owner
    gid_t  m_gid;  // group ID of owner
    mode_t      m_fileMode;  // file type & mode (permissions)
    FileType    m_fileType;
    std::string m_mimeType;
    dev_t m_dev;  // device number (file system)
    int  m_numLink;
    bool m_dirty;
    bool m_write;
    bool m_fileOpen;
    bool m_pendingGet;
    bool m_pendingCreate;

    FileMetaData(time_t atime, time_t mtime, uid_t uid, gid_t gid,
                 mode_t fileMode, FileType fileType = FileType::None,
                 std::string mimeType = "", dev_t dev = 0)
        : m_atime(atime),
          m_mtime(mtime),
          m_ctime(mtime),
          m_uid(uid),
          m_gid(gid),
          m_fileMode(fileMode),
          m_fileType(fileType),
          m_mimeType(mimeType),
          m_dev(dev),
          m_dirty(false),
          m_write(false),
          m_fileOpen(false),
          m_pendingGet(false),
          m_pendingCreate(false) {
      m_numLink = fileType == FileType::Directory
                       ? 2
                       : fileType == FileType::File ? 1 : 0;  // TODO
      m_cachedTime = time(NULL);
    }
  };

 public:
  Entry(const std::string &fileId, uint64_t fileSize, time_t atime,
        time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
        FileType fileType = FileType::None, std::string mimeType = "",
        dev_t dev = 0)
      : m_fileId(fileId),
        m_fileSize(fileSize),
        m_metaData(atime, mtime, uid, gid, fileMode, fileType, mimeType, dev) {}

  Entry(Entry &&) = default;
  Entry(const Entry &) = default;
  Entry &operator=(Entry &&) = default;
  Entry &operator=(const Entry &) = default;
  ~Entry() = default;

  operator bool() const {
    return !m_fileId.empty() && m_metaData.m_fileType != FileType::None;
  }

  bool IsDirectory() const {
    return m_metaData.m_fileType == FileType::Directory;
  }

  // accessor
  const std::string & GetFileId() const { return m_fileId; }
  uint64_t GetFileSize() const { return m_fileSize; }
  int GetNumLink() const { return m_metaData.m_numLink; }

 private:
  void DecreaseNumLink() { --m_metaData.m_numLink; }
  void IncreaseNumLink() { ++m_metaData.m_numLink; }

 private:
  std::string m_fileId;  // file path
  uint64_t m_fileSize;
  FileMetaData m_metaData;  // file meta data

  friend class Node;
};

/**
 * Representation of a Node in the directory tree.
 */
class Node {
 public:
  Node(char link, const std::string &fileName, std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent)
      : m_link(link),
        m_fileName(fileName),
        m_entry(std::move(entry)),
        m_parent(parent) {
    m_children.clear();
  }

  Node()
      : Node(0, "", std::unique_ptr<Entry>(nullptr),
             std::shared_ptr<Node>(nullptr)) {}

  Node(const std::string &fileName, std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent)
      : Node(0, fileName, std::move(entry), parent) {}

  Node(char link, const std::string &fileName, std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent, const char *symbolicLink)
      : Node(link, fileName, std::move(entry), parent) {
    if (entry && entry->GetFileSize() <= strlen(symbolicLink)) {
      m_symbolicLink = std::string(symbolicLink, entry->GetFileSize());
    }
  }

  Node(Node &&) = default;
  Node(const Node &) = delete;
  Node &operator=(Node &&) = default;
  Node &operator=(const Node &) = delete;

  ~Node() {
    m_entry->DecreaseNumLink();
    if (m_entry->GetNumLink() == 0 ||
        (m_entry->GetNumLink() <= 1 && m_entry->IsDirectory())) {
      m_entry.reset(nullptr);
    }
  }

 public:
  virtual operator bool() const { return m_entry->operator bool(); }

 public:
  bool IsEmpty() const;
  std::shared_ptr<Node> Find(const std::string &fileName) const;
  const StlFileNameToNodeMap  &GetChildren() const;

  std::shared_ptr<Node> Insert(std::shared_ptr<Node> child);
  void Remove(std::shared_ptr<Node> child);
  void RenameChild(const std::string &oldFileName,
                   const std::string &newFileName);

  //accessor
  const std::unique_ptr<Entry>& GetEntry() const {return m_entry;}
  const std::string& GetPath() const {return m_entry->GetFileId();}

 private:
  char m_link;  //TODO replace with FileType
  std::string m_fileName;
  std::unique_ptr<Entry> m_entry;  //TODO replace with std::shared_ptr
  std::weak_ptr<Node> m_parent;
  std::string m_symbolicLink;
  StlFileNameToNodeMap m_children;
};

}  // namespace FileSystem
}  // namespace QS

#endif  // FILESYSTEM_H_INCLUDED
