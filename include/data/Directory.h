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

#include <deque>
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

namespace Client {
class QSClient;
}  // namespace Client

namespace FileSystem {
class Drive;
}  // namespace FileSystem

namespace Data {

class Cache;
class File;

class DirectoryTree;
class Node;

using FilePathToNodeUnorderedMap =
    std::unordered_map<std::string, std::shared_ptr<Node>,
                       HashUtils::StringHash>;
using FilePathToWeakNodeUnorderedMap =
    std::unordered_map<std::string, std::weak_ptr<Node>, HashUtils::StringHash>;
using ParentFilePathToChildrenMultiMap =
    std::unordered_multimap<std::string, std::weak_ptr<Node>,
                            HashUtils::StringHash>;
using ChildrenMultiMapIterator = ParentFilePathToChildrenMultiMap::iterator;
using ChildrenMultiMapConstIterator =
    ParentFilePathToChildrenMultiMap::const_iterator;

class Entry {
 public:
  Entry(const std::string &filePath, uint64_t fileSize, time_t atime,
        time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
        FileType fileType = FileType::File, const std::string &mimeType = "",
        const std::string &eTag = std::string(), bool encrypted = false,
        dev_t dev = 0);

  explicit Entry(std::shared_ptr<FileMetaData> &&fileMetaData);

  explicit Entry(const std::shared_ptr<FileMetaData> &fileMetaData)
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
  bool IsSymLink() const {
    return m_metaData.lock()->m_fileType == FileType::SymLink;
  }

  // accessor
  const std::weak_ptr<FileMetaData> &GetMetaData() const { return m_metaData; }
  const std::string &GetFilePath() const {
    return m_metaData.lock()->m_filePath;
  }
  uint64_t GetFileSize() const { return m_metaData.lock()->m_fileSize; }
  int GetNumLink() const { return m_metaData.lock()->m_numLink; }
  FileType GetFileType() const { return m_metaData.lock()->m_fileType; }
  mode_t GetFileMode() const { return m_metaData.lock()->m_fileMode; }
  time_t GetMTime() const { return m_metaData.lock()->m_mtime; }
  uid_t GetUID() const { return m_metaData.lock()->m_uid; }
  bool IsNeedUpload() const { return m_metaData.lock()->m_needUpload; }
  bool IsFileOpen() const { return m_metaData.lock()->m_fileOpen; }

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
  void SetNeedUpload(bool needUpload) {
    m_metaData.lock()->m_needUpload = needUpload;
  }
  void SetFileOpen(bool fileOpen) { m_metaData.lock()->m_fileOpen = fileOpen; }

  void Rename(const std::string &newFilePath);

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
  // Ctor for root node which has no parent,
  // or for some case the parent is cleared or still not set at the time
  Node() : m_entry(Entry()), m_parent(std::shared_ptr<Node>(nullptr)) {}

  Node(Entry &&entry,
       const std::shared_ptr<Node> &parent = std::make_shared<Node>());

  Node(Entry &&entry, const std::shared_ptr<Node> &parent,
       const std::string &symbolicLink);

  // Not subclassing from enabled_shared_from_this, as cannot use
  // shared_from_this in ctors.
  // Node(Entry &&entry, std::shared_ptr<Node> &parent);
  // Node(Entry &&entry, std::shared_ptr<Node> &parent,
  //      const std::string &symbolicLink);

  Node(Node &&) = default;
  Node(const Node &) = delete;
  Node &operator=(Node &&) = default;
  Node &operator=(const Node &) = delete;
  ~Node();

 public:
  operator bool() const { return m_entry ? m_entry.operator bool() : false; }

  bool IsDirectory() const { return m_entry && m_entry.IsDirectory(); }
  bool IsSymLink() const { return m_entry && m_entry.IsSymLink(); }
  bool IsHardLink() const { return m_hardLink; }

 public:
  bool IsEmpty() const { return m_children.empty(); }
  bool HaveChild(const std::string &childFilePath) const;
  std::shared_ptr<Node> Find(const std::string &childFilePath) const;

  // Get Children
  const FilePathToNodeUnorderedMap &GetChildren()
      const;  // DO NOT store the map

  // Get the children's id (one level)
  std::set<std::string> GetChildrenIds() const;

  // Get the children file names recursively
  //
  // @param  : void
  // @return : a list of all children's file names and chilren's chidlren's ones
  //           in a recursively way. The nearest child is put at front.
  std::deque<std::string> GetChildrenIdsRecursively() const;

  std::shared_ptr<Node> Insert(const std::shared_ptr<Node> &child);
  void Remove(const std::shared_ptr<Node> &child);
  void Remove(const std::string &childFilePath);
  void RenameChild(const std::string &oldFilePath,
                   const std::string &newFilePath);

  // accessor
  const Entry &GetEntry() const { return m_entry; }
  std::shared_ptr<Node> GetParent() const { return m_parent.lock(); }
  std::string GetSymbolicLink() const { return m_symbolicLink; }

  std::string GetFilePath() const {
    return m_entry ? m_entry.GetFilePath() : std::string();
  }

  uint64_t GetFileSize() const { return m_entry ? m_entry.GetFileSize() : 0; }
  int GetNumLink() const { return m_entry ? m_entry.GetNumLink() : 0; }

  mode_t GetFileMode() const { return m_entry ? m_entry.GetFileMode() : 0; }
  time_t GetMTime() const { return m_entry ? m_entry.GetMTime() : 0; }
  uid_t GetUID() const { return m_entry ? m_entry.GetUID() : -1; }
  bool IsNeedUpload() const { return m_entry ? m_entry.IsNeedUpload() : false; }
  bool IsFileOpen() const { return m_entry ? m_entry.IsFileOpen() : false; }

  std::string MyDirName() const {
    return m_entry ? m_entry.MyDirName() : std::string();
  }
  std::string MyBaseName() const {
    return m_entry ? m_entry.MyBaseName() : std::string();
  }

  bool FileAccess(uid_t uid, gid_t gid, int amode) const {
    return m_entry ? m_entry.FileAccess(uid, gid, amode) : false;
  }

 private:
  Entry &GetEntry() { return m_entry; }

  void SetNeedUpload(bool needUpload) {
    if (m_entry) {
      m_entry.SetNeedUpload(needUpload);
    }
  }

  void SetFileOpen(bool fileOpen) {
    if (m_entry) {
      m_entry.SetFileOpen(fileOpen);
    }
  }

  void SetFileSize(size_t sz) {
    if (m_entry) {
      m_entry.SetFileSize(sz);
    }
  }

  void Rename(const std::string &newFilePath) {
    if (m_entry) {
      m_entry.Rename(newFilePath);
    }
  }

  void SetEntry(Entry &&entry) { m_entry = std::move(entry); }
  void SetParent(const std::shared_ptr<Node> &parent) { m_parent = parent; }
  void SetSymbolicLink(const std::string &symLnk) { m_symbolicLink = symLnk; }
  void SetHardLink(bool isHardLink) { m_hardLink = isHardLink; }

  void IncreaseNumLink() {
    if (m_entry) {
      m_entry.IncreaseNumLink();
    }
  }

 private:
  Entry m_entry;
  std::weak_ptr<Node> m_parent;
  std::string m_symbolicLink;
  bool m_hardLink = false;
  // Node will control the life of its children, so only Node hold a shared_ptr
  // to its children, others should use weak_ptr instead
  FilePathToNodeUnorderedMap m_children;

  friend class QS::Data::Cache;  // for GetEntry
  friend class QS::Data::DirectoryTree;
  friend class QS::FileSystem::Drive;  // for SetSymbolicLink, IncreaseNumLink
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
  // Get root
  std::shared_ptr<Node> GetRoot() const;

  // Get current node
  // removed as this may has effect on remove
  // std::shared_ptr<Node> GetCurrentNode() const;

  // Find node
  //
  // @param  : file path (absolute path)
  // @return : node
  std::weak_ptr<Node> Find(const std::string &filePath) const;

  // Return if dir tree has node
  //
  // @param  : file path (absolute path)
  // @return : node
  bool Has(const std::string &filePath) const;

  // Find children
  //
  // @param  : dir name which should be ending with "/"
  // @return : node list
  std::vector<std::weak_ptr<Node>> FindChildren(
      const std::string &dirName) const;

  // Const iterator point to begin of the parent to children map
  ChildrenMultiMapConstIterator CBeginParentToChildrenMap() const;

  // Const iterator point to end of the parent to children map
  ChildrenMultiMapConstIterator CEndParentToChildrenMap() const;

 private:
  // Grow the directory tree
  //
  // @param  : file meta data
  // @return : the Node referencing to the file meta data
  //
  // If the node reference to the meta data already exist, update meta data;
  // otherwise add node to the tree and build up the references.
  std::shared_ptr<Node> Grow(std::shared_ptr<FileMetaData> &&fileMeta);

  // Grow the directory tree
  //
  // @param  : file meta data list
  // @return : void
  //
  // This will walk through the meta data list to Grow the dir tree.
  void Grow(std::vector<std::shared_ptr<FileMetaData>> &&fileMetas);

  // Update a directory node in the directory tree
  //
  // @param  : dirpath, meta data of children
  // @return : the node has been update or null if update doesn't happen
  std::shared_ptr<Node> UpdateDirectory(
      const std::string &dirPath,
      std::vector<std::shared_ptr<FileMetaData>> &&childrenMetas);

  // Rename node
  //
  // @param  : old file path, new file path (absolute path)
  // @return : the node has been renamed or null if rename doesn't happen
  std::shared_ptr<Node> Rename(const std::string &oldFilePath,
                               const std::string &newFilePath);

  // Remove node
  //
  // @param  : path
  // @return : void
  //
  // This will remove node and all its childrens (recursively)
  void Remove(const std::string &path);

  // Creat a hard link to a file
  //
  // @param  : the file path, the hard link path
  // @return : the node of hard link to the file or null if fail to link
  std::shared_ptr<Node> HardLink(const std::string &filePath,
                                 const std::string &hardlinkPath);

 private:
  std::shared_ptr<Node> m_root;
  // std::shared_ptr<Node> m_currentNode;
  mutable std::recursive_mutex m_mutex;
  FilePathToWeakNodeUnorderedMap m_map;  // record all nodes map

  // As we grow directory tree gradually, that means the directory tree can
  // be a partial part of the entire tree, at some point some nodes haven't
  // built the reference to its parent or children because which have not been
  // added to the tree yet.
  // So, the dirName to children map which will help to update these references.
  ParentFilePathToChildrenMultiMap m_parentToChildrenMap;

  friend class QS::Client::QSClient;
  friend class QS::FileSystem::Drive;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_DATA_DIRECTORY_H_
