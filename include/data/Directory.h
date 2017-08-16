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

#include <sys/stat.h>
#include <unistd.h>

#include <memory>
#include <mutex>  // NOLINT
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/HashUtils.h"
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

using FilePathToNodeUnorderedMap =
    std::unordered_map<std::string, std::shared_ptr<Node>, HashUtils::StringHash>;
using FilePathToWeakNodeUnorderedMap =
    std::unordered_map<std::string, std::weak_ptr<Node>, HashUtils::StringHash>;
using ParentFilePathToChildrenMultiMap =
    std::unordered_multimap<std::string, std::weak_ptr<Node>,
                            HashUtils::StringHash>;
using ChildrenMultiMapIterator = ParentFilePathToChildrenMultiMap::iterator;
using ChildrenMultiMapConstIterator = ParentFilePathToChildrenMultiMap::const_iterator;

class Entry {
 public:
  Entry(const std::string &filePath, uint64_t fileSize, time_t atime,
        time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
        FileType fileType = FileType::File, const std::string &mimeType = "",
        const std::string &eTag = std::string(), bool encrypted = false,
        dev_t dev = 0);

  Entry(std::shared_ptr<FileMetaData> &&fileMetaData);

  Entry(const std::shared_ptr<FileMetaData> &fileMetaData)
      : m_metaData(fileMetaData) {}

  Entry() = default;
  Entry(Entry &&) = default;
  Entry(const Entry &) = default;
  Entry &operator=(Entry &&) = default;
  Entry &operator=(const Entry &) = default;
  ~Entry() = default;

  // You always need to check if the entry is operable before 
  // invoke its member functions
  operator bool() const {
    auto meta = m_metaData.lock();
    return meta ? !meta->m_filePath.empty() : false;
  }

   bool IsDirectory() const {
    return m_metaData.lock()->m_fileType == FileType::Directory;
  }

  // accessor
  const std::weak_ptr<FileMetaData> &GetMetaData() const { return m_metaData; }
  const std::string &GetFilePath() const {
    return m_metaData.lock()->m_filePath;
  }
  uint64_t GetFileSize() const { return m_metaData.lock()->m_fileSize; }
  int GetNumLink() const { return m_metaData.lock()->m_numLink; }
  FileType GetFileType() const { return m_metaData.lock()->m_fileType; }
  time_t GetMTime() const { return m_metaData.lock()->m_mtime; }

  std::string MyDirName() const { return m_metaData.lock()->MyDirName(); }
  std::string MyBaseName() const { return m_metaData.lock()->MyBaseName(); }

  struct stat ToStat() const {
    return m_metaData.lock()->ToStat();
  }

  bool FileAccess(uid_t uid, gid_t gid, int amode) const {
    return m_metaData.lock()->FileAccess(uid, gid, amode);
  }

 private:
  void DecreaseNumLink() { --m_metaData.lock()->m_numLink; }
  void IncreaseNumLink() { ++m_metaData.lock()->m_numLink; }
  void SetFileSize(uint64_t size) { m_metaData.lock()->m_fileSize = size; }
  void SetFilePath(const std::string &newFilePath) {
    m_metaData.lock()->m_filePath = newFilePath;
  }
 
 private:
  // Using weak_ptr as FileMetaDataManger will control file mete data life cycle
  std::weak_ptr<FileMetaData> m_metaData;  // file meta data

  friend class Node;
  friend class QS::Data::File;  // for SetFileSize
};

/**
 * Representation of a Node in the directory tree.
 */
class Node {
 public:
  Node(Entry &&entry,
       const std::shared_ptr<Node> &parent = std::make_shared<Node>())
      : m_entry(std::move(entry)), m_parent(parent) {
    m_children.clear();
  }

  // Ctor for root node which has no parent,
  // or for some case the parent is cleared or still not set at the time
  Node()
      : Node(Entry(), std::shared_ptr<Node>(nullptr)) {}

  Node(Entry &&entry, const std::shared_ptr<Node> &parent,
       const std::string &symbolicLink)
      : Node(std::move(entry), parent) {
    // must use m_entry instead of entry which is moved to m_entry now
    if (m_entry && m_entry.GetFileSize() <= symbolicLink.size()) {
      m_symbolicLink = std::string(symbolicLink, 0, m_entry.GetFileSize());
    }
  }

  Node(Node &&) = default;
  Node(const Node &) = delete;
  Node &operator=(Node &&) = default;
  Node &operator=(const Node &) = delete;
  ~Node();

 public:
  virtual operator bool() const {
    return m_entry ? m_entry.operator bool() : false;
  }

  bool IsDirectory() const {
    return m_entry && m_entry.IsDirectory();
  }

 public:
  bool IsEmpty() const { return m_children.empty(); }
  std::shared_ptr<Node> Find(const std::string &childFilePath) const;
  const FilePathToNodeUnorderedMap &GetChildren() const;
  std::set<std::string> GetChildrenIds() const;

  std::shared_ptr<Node> Insert(const std::shared_ptr<Node> &child);
  void Remove(const std::shared_ptr<Node> &child);
  void Remove(const std::string& childFilePath);
  void RenameChild(const std::string &oldFilePath,
                   const std::string &newFilePath);

  // accessor
  const Entry &GetEntry() const { return m_entry; }
  std::shared_ptr<Node> GetParent() const { return m_parent.lock(); }
  std::string GetSymbolicLink() const { return m_symbolicLink; }

  std::string GetFilePath() const {
    if (!m_entry) return std::string();
    return m_entry.GetFilePath();
  }

  std::string MyDirName() const {
    if (!m_entry) return std::string();
    return m_entry.MyDirName();
  }

  std::string MyBaseName() const {
    if (!m_entry) return std::string();
    return m_entry.MyBaseName();
  }

  bool FileAccess(uid_t uid, gid_t gid, int amode) const {
    return m_entry ? m_entry.FileAccess(uid, gid, amode) : false;
  }

 private:
  Entry &GetEntry() { return m_entry; }

  void SetFilePath(const std::string &newFilePath) {
    if (m_entry) {
      m_entry.SetFilePath(newFilePath);
    }
  }

  void SetEntry(Entry &&entry) { m_entry = std::move(entry); }
  void SetParent(const std::shared_ptr<Node> &parent) { m_parent = parent; }

 private:
  Entry m_entry;
  std::weak_ptr<Node> m_parent;
  std::string m_symbolicLink;  // TODO(jim): meaningful?
  FilePathToNodeUnorderedMap m_children;

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
  // TODO(jim):
  // GetRoot
  // UpdateNode
  // Find Node by file full path
  std::weak_ptr<Node> Find(const std::string &filePath) const;
  // Find children by dirname which should be ending with "/"
  std::pair<ChildrenMultiMapConstIterator, ChildrenMultiMapConstIterator>
  FindChildren(const std::string &dirName) const;

  ChildrenMultiMapConstIterator CBeginParentToChildrenMap() const;
  ChildrenMultiMapConstIterator CEndParentToChildrenMap() const;

 private:
  // Grow the directory tree.
  // If the node reference to the meta data already exist, update meta data;
  // otherwise add node to the tree and build up the references.
  void Grow(std::shared_ptr<FileMetaData> &&fileMeta);
  void Grow(std::vector<std::shared_ptr<FileMetaData>> &&fileMetas);

  // Update a directory node in the directory tree.
  void UpdateDiretory(
      const std::string &dirPath,
      std::vector<std::shared_ptr<FileMetaData>> &&childrenMetas);

 private:
  std::shared_ptr<Node> m_root;
  std::shared_ptr<Node> m_currentNode;
  mutable std::recursive_mutex m_mutex;
  FilePathToWeakNodeUnorderedMap m_map;  // record all nodes map
  // As we grow directory tree gradually, that means the directory tree can
  // be a partial part of the entire tree, at some point some nodes haven't built
  // the reference to its parent or children because which have not been added to 
  // the tree yet.
  // So, the dirName to children map which will help to update these references.
  ParentFilePathToChildrenMultiMap m_parentToChildrenMap;

  friend class QS::FileSystem::Drive;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_
