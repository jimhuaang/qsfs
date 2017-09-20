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

#include "data/Directory.h"

#include <cassert>

#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
#include <queue>
#include <set>
#include <string>

#include <utility>

#include "base/LogMacros.h"
#include "base/Utils.h"
#include "data/FileMetaDataManager.h"

namespace QS {

namespace Data {

using QS::Utils::AppendPathDelim;
using QS::Utils::IsRootDirectory;
using std::deque;
using std::lock_guard;
using std::make_shared;
using std::queue;
using std::recursive_mutex;
using std::set;
using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;

static const char *const ROOT_PATH = "/";

// --------------------------------------------------------------------------
Entry::Entry(const std::string &filePath, uint64_t fileSize, time_t atime,
             time_t mtime, uid_t uid, gid_t gid, mode_t fileMode,
             FileType fileType, const std::string &mimeType,
             const std::string &eTag, bool encrypted, dev_t dev) {
  auto meta = make_shared<FileMetaData>(filePath, fileSize, atime, mtime, uid,
                                        gid, fileMode, fileType, mimeType, eTag,
                                        encrypted, dev);
  m_metaData = meta;
  FileMetaDataManager::Instance().Add(std::move(meta));
}

// --------------------------------------------------------------------------
Entry::Entry(std::shared_ptr<FileMetaData> &&fileMetaData)
    : m_metaData(fileMetaData) {
  FileMetaDataManager::Instance().Add(std::move(fileMetaData));
}

// --------------------------------------------------------------------------
Node::Node(Entry &&entry, const shared_ptr<Node> &parent)
    : m_entry(std::move(entry)), m_parent(parent) {
  m_children.clear();
}

// --------------------------------------------------------------------------
Node::Node(Entry &&entry, const shared_ptr<Node> &parent,
           const string &symbolicLink)
    : Node(std::move(entry), parent) {
  // must use m_entry instead of entry which is moved to m_entry now
  if (m_entry && m_entry.GetFileSize() <= symbolicLink.size()) {
    m_symbolicLink = std::string(symbolicLink, 0, m_entry.GetFileSize());
  }
}

// --------------------------------------------------------------------------
Node::~Node() {
  if (!m_entry) return;

  if (IsDirectory() || IsHardLink()) {
    auto parent = m_parent.lock();
    if (parent) {
      parent->GetEntry().DecreaseNumLink();
    }
  }

  GetEntry().DecreaseNumLink();
  if (m_entry.GetNumLink() <= 0 ||
      (m_entry.GetNumLink() <= 1 && m_entry.IsDirectory())) {
    FileMetaDataManager::Instance().Erase(GetFilePath());
  }
}

// --------------------------------------------------------------------------
shared_ptr<Node> Node::Find(const string &childFileName) const {
  auto child = m_children.find(childFileName);
  if (child != m_children.end()) {
    return child->second;
  }
  return shared_ptr<Node>(nullptr);
}

// --------------------------------------------------------------------------
bool Node::HaveChild(const std::string &childFilePath) const {
  return m_children.find(childFilePath) != m_children.end();
}

// --------------------------------------------------------------------------
const FilePathToNodeUnorderedMap &Node::GetChildren() const {
  return m_children;
}

// --------------------------------------------------------------------------
set<string> Node::GetChildrenIds() const {
  set<string> ids;
  for (const auto &pair : m_children) {
    ids.emplace(pair.first);
  }
  return ids;
}

// --------------------------------------------------------------------------
deque<string> Node::GetChildrenIdsRecursively() const {
  deque<string> ids;
  deque<shared_ptr<Node>> childs;

  for (const auto &pair : m_children) {
    ids.emplace_back(pair.first);
    childs.push_back(pair.second);
  }

  while (!childs.empty()) {
    auto child = childs.front();
    childs.pop_front();

    if (child->IsDirectory()) {
      for (const auto &pair : child->GetChildren()) {
        ids.emplace_back(pair.first);
        childs.push_back(pair.second);
      }
    }
  }

  return ids;
}

// --------------------------------------------------------------------------
shared_ptr<Node> Node::Insert(const shared_ptr<Node> &child) {
  assert(IsDirectory());
  if (child) {
    auto res = m_children.emplace(child->GetFilePath(), child);
    if (res.second) {
      if (child->IsDirectory()) {
        m_entry.IncreaseNumLink();
      }
    } else {
      DebugInfo(child->GetFilePath() +
                " is already existed, no insertion happens");
    }
  } else {
    DebugWarning("Try to insert null Node. Go on");
  }
  return child;
}

// --------------------------------------------------------------------------
void Node::Remove(const shared_ptr<Node> &child) {
  if (child) {
    Remove(child->GetFilePath());
  } else {
    DebugWarning("Try to remove null Node. Go on")
  }
}

// --------------------------------------------------------------------------
void Node::Remove(const std::string &childFilePath) {
  if (childFilePath.empty()) return;

  bool reset = m_children.size() == 1 ? true : false;
  auto it = m_children.find(childFilePath);
  if (it != m_children.end()) {
    m_children.erase(it);
    if (reset) m_children.clear();
  } else {
    DebugWarning("Try to remove Node " + childFilePath +
                 " which is not found. Go on");
  }
}

// --------------------------------------------------------------------------
void Node::RenameChild(const string &oldFilePath, const string &newFilePath) {
  if (oldFilePath == newFilePath) {
    DebugInfo("New file name is the same as the old one. Go on");
    return;
  }

  if (m_children.find(newFilePath) != m_children.end()) {
    DebugWarning("Cannot rename " + oldFilePath + " to " + newFilePath +
                 " which is already existed. But continue...");
    return;
  }

  auto it = m_children.find(oldFilePath);
  if (it != m_children.end()) {
    auto tmp = it->second;
    tmp->SetFilePath(newFilePath);
    auto hint = m_children.erase(it);
    m_children.emplace_hint(hint, newFilePath, tmp);
  } else {
    DebugWarning("Try to rename Node " + oldFilePath +
                 " which is not found. Go on");
  }
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::GetRoot() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_root;
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::GetCurrentNode() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_currentNode;
}

// --------------------------------------------------------------------------
weak_ptr<Node> DirectoryTree::Find(const string &filePath) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto it = m_map.find(filePath);
  if (it != m_map.end()) {
    return it->second;
  } else {
    DebugInfo("Node (" + filePath + ") is not existing in directory tree");
    return weak_ptr<Node>();
  }
}

// --------------------------------------------------------------------------
std::pair<ChildrenMultiMapConstIterator, ChildrenMultiMapConstIterator>
DirectoryTree::FindChildren(const string &dirName) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_parentToChildrenMap.equal_range(dirName);
}

// --------------------------------------------------------------------------
ChildrenMultiMapConstIterator DirectoryTree::CBeginParentToChildrenMap() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_parentToChildrenMap.cbegin();
}

// --------------------------------------------------------------------------
ChildrenMultiMapConstIterator DirectoryTree::CEndParentToChildrenMap() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_parentToChildrenMap.cend();
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::Grow(shared_ptr<FileMetaData> &&fileMeta) {
  lock_guard<recursive_mutex> lock(m_mutex);
  if(!fileMeta) return nullptr;

  string filePath = fileMeta->GetFilePath();

  auto node = Find(filePath).lock();
  if (node) {
    if(fileMeta->GetMTime() > node->GetMTime()){
      node->SetEntry(Entry(std::move(fileMeta)));  // update entry
    }
  } else {
    bool isDir = fileMeta->IsDirectory();
    auto dirName = fileMeta->MyDirName();
    node = make_shared<Node>(Entry(std::move(fileMeta)));
    m_map.emplace(filePath, node);

    // hook up with parent
    assert(!dirName.empty());
    auto it = m_map.find(dirName);
    if (it != m_map.end()) {
      if (auto parent = it->second.lock()) {
        parent->Insert(node);
        node->SetParent(parent);
      } else {
        DebugInfo("Parent Node of " + filePath +
                  " is not available at the time in directory tree");
      }
    }

    // hook up with children
    if (isDir) {
      auto range = m_parentToChildrenMap.equal_range(filePath);
      for (auto it = range.first; it != range.second; ++it) {
        if (auto child = it->second.lock()) {
          child->SetParent(node);
          node->Insert(child);
        }
      }
    }

    // record parent to children map
    m_parentToChildrenMap.emplace(dirName, node);
  }
  m_currentNode = node;

  return node;
}

// --------------------------------------------------------------------------
void DirectoryTree::Grow(vector<shared_ptr<FileMetaData>> &&fileMetas) {
  lock_guard<recursive_mutex> lock(m_mutex);
  for (auto &meta : fileMetas) {
    Grow(std::move(meta));
  }
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::UpdateDiretory(
    const string &dirPath, vector<shared_ptr<FileMetaData>> &&childrenMetas) {
  if (dirPath.empty()) {
    DebugWarning("Null dir path");
    return shared_ptr<Node>(nullptr);
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  DebugInfo("Update directory of " + dirPath);
  string path = dirPath;
  if (dirPath.back() != '/') {
    DebugInfo("Input dir path is not ending with '/', append it");
    path = AppendPathDelim(dirPath);
  }
  // Check children metas and collect valid ones
  vector<shared_ptr<FileMetaData>> newChildrenMetas;
  set<string> newChildrenIds;
  for (auto &child : childrenMetas) {
    auto childDirName = child->MyDirName();
    if (childDirName.empty()) {
      DebugWarning("Invalid child meta data with empty dirname");
      continue;
    }
    if (childDirName != path) {
      DebugWarning("Invalid child meta data with dirname=" + childDirName +
                   " which is diffrent with " + path);
      continue;
    }
    newChildrenIds.emplace(child->GetFilePath());
    newChildrenMetas.push_back(std::move(child));
  }

  auto node = Find(path).lock();
  if (!node) {  // directory not existing
    node = Grow(std::move(BuildDefaultDirectoryMeta(path)));
    Grow(std::move(newChildrenMetas));
  } else {
    if (!node->IsDirectory()) {
      DebugError("Found Node is not a directory for path=" + path);
      return shared_ptr<Node>(nullptr);
    }

    // Do deleting
    set<string> oldChildrenIds = node->GetChildrenIds();
    set<string> deleteChildrenIds;
    std::set_difference(
        oldChildrenIds.begin(), oldChildrenIds.end(), newChildrenIds.begin(),
        newChildrenIds.end(),
        std::inserter(deleteChildrenIds, deleteChildrenIds.end()));
    if (!deleteChildrenIds.empty()) {
      auto range = FindChildren(path);
      for (auto it = range.first; it != range.second; ++it) {
        auto childNode = it->second.lock();
        if (childNode && (*childNode)) {
          if (deleteChildrenIds.find(childNode->GetFilePath()) !=
              deleteChildrenIds.end()) {
            m_parentToChildrenMap.erase(it);
          }
        }
      }
      for (auto &childId : deleteChildrenIds) {
        m_map.erase(childId);
        node->Remove(childId);
      }
    }

    // Do updating
    Grow(std::move(newChildrenMetas));
  }
  m_currentNode = node;
  return node;
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::Rename(const string &oldFilePath,
                                       const string &newFilePath) {
  if (oldFilePath.empty() || newFilePath.empty()) {
    DebugWarning("Null input parameter [old file: " + oldFilePath +
                 ", new file: " + newFilePath + "]");
    return shared_ptr<Node>(nullptr);
  }
  if (IsRootDirectory(oldFilePath)) {
    DebugError("Unable to rename root");
    return shared_ptr<Node>(nullptr);
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(oldFilePath).lock();
  if (node) {
    // Check parameter
    if (Find(newFilePath).lock()) {
      DebugWarning("Cannot renameing, Node is existing for new file path " +
                   newFilePath);
      return node;
    }
    if (!(*node)) {
      DebugError("Node is not operable for file path " + oldFilePath);
      return node;
    }

    // Do Renaming
    auto parentName = node->MyDirName();
    node->SetFilePath(newFilePath);
    auto parent = node->GetParent();
    if (parent) {
      parent->RenameChild(oldFilePath, newFilePath);
    }
    m_map.emplace(newFilePath, node);
    m_map.erase(oldFilePath);
    if (node->IsDirectory()) {
      auto range = FindChildren(newFilePath);
      for (auto it = range.first; it != range.second; ++it) {
        m_parentToChildrenMap.emplace(newFilePath, std::move(it->second));
        m_parentToChildrenMap.erase(it);
      }
    }
    m_currentNode = node;
  } else {
    DebugWarning("Node is not existing for " + oldFilePath);
  }

  return node;
}

// --------------------------------------------------------------------------
void DirectoryTree::Remove(const string &path) {
  if (IsRootDirectory(path)) {
    DebugError("Unable to remove root");
    return;
  }
  
  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(path).lock();
  if (!(node && *node)) {
    DebugInfo("No such file or directory " + path);
    return;
  }

  auto parent = node->GetParent();
  if (parent) {
    parent->Remove(path);
  }
  m_map.erase(path);
  m_parentToChildrenMap.erase(path);

  if(!node->IsDirectory()){
    node.reset();
    return;
  }
  
  std::queue<shared_ptr<Node>> deleteNodes;
  for (auto &pair : node->GetChildren()) {
    deleteNodes.push(std::move(pair.second));
  }
  // recursively remove all children references
  while(!deleteNodes.empty()){
    auto node_ = deleteNodes.front();
    deleteNodes.pop();

    auto path_ = node_->GetFilePath();
    m_map.erase(path_);
    m_parentToChildrenMap.erase(path_);

    if(node->IsDirectory()){
      for(auto &pair : node_->GetChildren()){
        deleteNodes.push(std::move(pair.second));
      }
    }
  }

  // if path is a directory, when go out of this function, destructor
  // will recursively delete all its children, as there is no references
  // to the node now.
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::HardLink(const string &filePath,
                                         const string &hardlinkPath) {
  // DO not use this for now.
  // HardLink currently shared meta data of target file when creating it,
  // Still need to synchronize with target file, to support this we may need
  // to refactory Node to contain a shared_ptr<Entry>.
  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(filePath).lock();
  if (!(node && *node)) {
    DebugError("No such file " + filePath);
    return shared_ptr<Node>(nullptr);
  }
  if (node->IsDirectory()) {
    DebugError("Unable to hard link to a directory [path=" + filePath +
               ", link=" + hardlinkPath);
    return shared_ptr<Node>(nullptr);
  }

  auto lnkNode = make_shared<Node>(Entry(node->GetEntry()), node);
  if (!(lnkNode && *lnkNode)) {
    DebugError("Fail to create a hard link [path=" + filePath +
               ", link=" + hardlinkPath);
    return shared_ptr<Node>(nullptr);
  }
  lnkNode->SetHardLink(true);
  node->Insert(lnkNode);
  node->IncreaseNumLink();
  m_map.emplace(hardlinkPath, lnkNode);
  m_parentToChildrenMap.emplace(node->GetFilePath(), lnkNode);
  m_currentNode = lnkNode;
  return lnkNode;
}

// --------------------------------------------------------------------------
DirectoryTree::DirectoryTree(time_t mtime, uid_t uid, gid_t gid, mode_t mode) {
  lock_guard<recursive_mutex> lock(m_mutex);
  m_root = make_shared<Node>(
      Entry(ROOT_PATH, 0, mtime, mtime, uid, gid, mode, FileType::Directory));
  m_currentNode = m_root;
  m_map.emplace(ROOT_PATH, m_root);
}

}  // namespace Data
}  // namespace QS
