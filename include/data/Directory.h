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
#include <vector>

#include "base/Utils.h"
#include "data/FileMetaData.h"

namespace QS {

namespace FileSystem {

class Drive;
}  // namespace FileSystem

namespace Data {

class Cache;
class File;

class DirectoryTree;
class Node;

using FileNameToNodeUnorderedMap =
    std::unordered_map<std::string, std::shared_ptr<Node>, Utils::StringHash>;
using FileNameToWeakNodeUnorderedMap =
    std::unordered_map<std::string, std::weak_ptr<Node>, Utils::StringHash>;
using ParentFileNameToChildrenMultiMap =
    std::unordered_multimap<std::string, std::weak_ptr<Node>,
                            Utils::StringHash>;

class Entry {
 public:
 public:
  Entry(const std::string &fileName, uint64_t fileSize, time_t atime,
        time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
        FileType fileType = FileType::File, const std::string &mimeType = "",
        const std::string &eTag = std::string(), bool encrypted = false,
        dev_t dev = 0)
      : m_metaData(std::make_shared<FileMetaData>(
            fileName, fileSize, atime, mtime, uid, gid, fileMode, fileType,
            mimeType, eTag, encrypted, dev)) {}

  Entry(const std::shared_ptr<FileMetaData> &fileMetaData)
      : m_metaData(fileMetaData) {}

  Entry() = default;
  Entry(Entry &&) = default;
  Entry(const Entry &) = default;
  Entry &operator=(Entry &&) = default;
  Entry &operator=(const Entry &) = default;
  ~Entry() = default;

  operator bool() const { return !m_metaData->m_fileName.empty(); }

  bool IsDirectory() const {
    return m_metaData->m_fileType == FileType::Directory;
  }

  // accessor
  const std::string &GetFileName() const { return m_metaData->m_fileName; }
  uint64_t GetFileSize() const { return m_metaData->m_fileSize; }
  int GetNumLink() const { return m_metaData->m_numLink; }
  FileType GetFileType() const { return m_metaData->m_fileType; }
  time_t GetMTime() const { return m_metaData->m_mtime; }

 private:
  void DecreaseNumLink() { --m_metaData->m_numLink; }
  void IncreaseNumLink() { ++m_metaData->m_numLink; }
  void SetFileSize(uint64_t size) { m_metaData->m_fileSize = size; }
  void SetFileName(const std::string &newFileName) {
    m_metaData->m_fileName = newFileName;
  }

 private:
  // TODO(jim): change to weak_ptr otherwise this will prevent FileMetaDataManger to clear it memory
  std::shared_ptr<FileMetaData> m_metaData;  // file meta data

  friend class Node;
  friend class QS::Data::File;  // for SetFileSize
};

/**
 * Representation of a Node in the directory tree.
 */
class Node {
 public:
  Node(std::unique_ptr<Entry> entry,
       const std::shared_ptr<Node> &parent = std::make_shared<Node>())
      : m_entry(std::move(entry)), m_parent(parent) {
    m_children.clear();
  }

  // Ctor for root node which has no parent,
  // or for some case the parent is cleared or still not set at the time
  Node()
      : Node(std::unique_ptr<Entry>(nullptr), std::shared_ptr<Node>(nullptr)) {}

  Node(std::unique_ptr<Entry> entry, const std::shared_ptr<Node> &parent,
       const std::string &symbolicLink)
      : Node(std::move(entry), parent) {
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
  std::shared_ptr<Node> Find(const std::string &childFileName) const;
  const FileNameToNodeUnorderedMap &GetChildren() const;

  std::shared_ptr<Node> Insert(const std::shared_ptr<Node> &child);
  void Remove(const std::shared_ptr<Node> &child);
  void RenameChild(const std::string &oldFileName,
                   const std::string &newFileName);

  // accessor
  const std::unique_ptr<Entry> &GetEntry() const { return m_entry; }
  std::weak_ptr<Node> GetParent() const { return m_parent; }
  std::string GetSymbolicLink() const { return m_symbolicLink; }

  std::string GetFileName() const {
    if (!m_entry) return std::string();
    return m_entry->GetFileName();
  }

 private:
  std::unique_ptr<Entry> &GetEntry() { return m_entry; }

  void SetFileName(const std::string &newFileName) {
    if (m_entry) {
      m_entry->SetFileName(newFileName);
    }
  }

  void SetParent(const std::shared_ptr<Node> &parent) { m_parent = parent; }

 private:
  std::unique_ptr<Entry> m_entry;
  std::weak_ptr<Node> m_parent;
  std::string m_symbolicLink;  // TODO(jim): meaningful?
  FileNameToNodeUnorderedMap m_children;

  friend class QS::Data::Cache;  // for GetEntry
  friend class QS::Data::DirectoryTree;
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
  void Grow(std::shared_ptr<FileMetaData> &&fileMeta);
  void Grow(std::vector<std::shared_ptr<FileMetaData>> &&fileMetas);

 private:
  std::shared_ptr<Node> m_root;
  std::shared_ptr<Node> m_currentNode;
  std::recursive_mutex m_mutex;
  FileNameToWeakNodeUnorderedMap m_map;  // record all nodes map
  ParentFileNameToChildrenMultiMap m_parentToChildrenMap;

  friend class QS::FileSystem::Drive;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_
