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
#include <vector>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "data/FileMetaDataManager.h"

namespace QS {

namespace Data {

using QS::StringUtils::FormatPath;
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
void Entry::Rename(const std::string &newFilePath) {
  FileMetaDataManager::Instance().Rename(GetFilePath(), newFilePath);
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
      DebugInfo("Node already exists, no insertion " +
                FormatPath(child->GetFilePath()));
    }
  } else {
    DebugWarning("Try to insert null Node");
  }
  return child;
}

// --------------------------------------------------------------------------
void Node::Remove(const shared_ptr<Node> &child) {
  if (child) {
    Remove(child->GetFilePath());
  } else {
    DebugWarning("Try to remove null Node")
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
    DebugWarning("Node not exist, no remove " + FormatPath(childFilePath));
  }
}

// --------------------------------------------------------------------------
void Node::RenameChild(const string &oldFilePath, const string &newFilePath) {
  if (oldFilePath == newFilePath) {
    DebugInfo("Same file name, no rename " + FormatPath(oldFilePath));
    return;
  }

  if (m_children.find(newFilePath) != m_children.end()) {
    DebugWarning("Cannot rename, target node already exist " +
                 FormatPath(oldFilePath, newFilePath));
    return;
  }

  auto it = m_children.find(oldFilePath);
  if (it != m_children.end()) {
    auto child = it->second;
    child->Rename(newFilePath);

    auto hint = m_children.erase(it);
    m_children.emplace_hint(hint, newFilePath, child);
  } else {
    DebugWarning("Node not exist, no rename " + FormatPath(oldFilePath));
  }
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::GetRoot() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_root;
}

// --------------------------------------------------------------------------
// shared_ptr<Node> DirectoryTree::GetCurrentNode() const {
//   lock_guard<recursive_mutex> lock(m_mutex);
//   return m_currentNode;
// }

// --------------------------------------------------------------------------
weak_ptr<Node> DirectoryTree::Find(const string &filePath) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto it = m_map.find(filePath);
  if (it != m_map.end()) {
    return it->second;
  } else {
    // Too many info, so disable it
    // DebugInfo("Node (" + filePath + ") is not existing in directory tree");
    return weak_ptr<Node>();
  }
}

// --------------------------------------------------------------------------
bool DirectoryTree::Has(const std::string &filePath) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_map.find(filePath) != m_map.end();
}

// --------------------------------------------------------------------------
vector<weak_ptr<Node>> DirectoryTree::FindChildren(
    const string &dirName) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto range = m_parentToChildrenMap.equal_range(dirName);
  vector<weak_ptr<Node>> childs;
  for (auto it = range.first; it != range.second; ++it) {
    childs.emplace_back(it->second);
  }
  return childs;
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
  if (!fileMeta) return nullptr;

  string filePath = fileMeta->GetFilePath();

  auto node = Find(filePath).lock();
  if (node && *node) {
    if (fileMeta->GetMTime() > node->GetMTime()) {
      DebugInfo("Update Node " + FormatPath(filePath));
      node->SetEntry(Entry(std::move(fileMeta)));  // update entry
    }
  } else {
    DebugInfo("Add Node " + FormatPath(filePath));
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
        DebugInfo("Parent node not exist " + FormatPath(filePath));
      }
    }

    // hook up with children
    if (isDir) {
      auto childs = FindChildren(filePath);
      for (auto &child : childs) {
        auto childNode = child.lock();
        if (childNode) {
          childNode->SetParent(node);
          node->Insert(childNode);
        }
      }
    }

    // record parent to children map
    m_parentToChildrenMap.emplace(dirName, node);
  }
  // m_currentNode = node;

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
shared_ptr<Node> DirectoryTree::UpdateDirectory(
    const string &dirPath, vector<shared_ptr<FileMetaData>> &&childrenMetas) {
  if (dirPath.empty()) {
    DebugWarning("Null dir path");
    return shared_ptr<Node>(nullptr);
  }
  string path = dirPath;
  if (dirPath.back() != '/') {
    DebugInfo("Input dir path is not ending with '/', append it");
    path = AppendPathDelim(dirPath);
  }

  DebugInfo("Update directory " + FormatPath(dirPath));
  lock_guard<recursive_mutex> lock(m_mutex);
  // Check children metas and collect valid ones
  vector<shared_ptr<FileMetaData>> newChildrenMetas;
  set<string> newChildrenIds;
  for (auto &child : childrenMetas) {
    auto childDirName = child->MyDirName();
    auto childFilePath = child->GetFilePath();
    if (childDirName.empty()) {
      DebugWarning("Invalid child Node " + FormatPath(childFilePath) +
                   " has empty dirname");
      continue;
    }
    if (childDirName != path) {
      DebugWarning("Invalid child Node " + FormatPath(childFilePath) +
                   " has different dir with " + path);
      continue;
    }
    newChildrenIds.emplace(childFilePath);
    newChildrenMetas.push_back(std::move(child));
  }

  // Update
  auto node = Find(path).lock();
  if (node && *node) {
    if (!node->IsDirectory()) {
      DebugWarning("Not a directory " + FormatPath(path));
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
      auto childs = FindChildren(path);
      m_parentToChildrenMap.erase(path);
      for (auto &child : childs) {
        auto childNode = child.lock();
        if (childNode && (*childNode)) {
          if (deleteChildrenIds.find(childNode->GetFilePath()) ==
              deleteChildrenIds.end()) {
            m_parentToChildrenMap.emplace(path, std::move(child));
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
  } else {  // directory not existing
    node = Grow(std::move(BuildDefaultDirectoryMeta(path)));
    Grow(std::move(newChildrenMetas));
  }

  // m_currentNode = node;
  return node;
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::Rename(const string &oldFilePath,
                                       const string &newFilePath) {
  if (oldFilePath.empty() || newFilePath.empty()) {
    DebugWarning("Cannot rename " + FormatPath(oldFilePath, newFilePath));
    return shared_ptr<Node>(nullptr);
  }
  if (IsRootDirectory(oldFilePath)) {
    DebugWarning("Unable to rename root");
    return shared_ptr<Node>(nullptr);
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(oldFilePath).lock();
  if (node && *node) {
    // Check parameter
    if (Find(newFilePath).lock()) {
      DebugWarning("Node exist, no rename " + FormatPath(newFilePath));
      return node;
    }
    if (!(*node)) {
      DebugWarning("Node not operable, no rename " + FormatPath(oldFilePath));
      return node;
    }

    // Do Renaming
    DebugInfo("Rename Node " + FormatPath(oldFilePath, newFilePath));
    auto parentName = node->MyDirName();
    node->Rename(newFilePath);  // still need as parent maybe not added yet
    auto parent = node->GetParent();
    if (parent && *parent) {
      parent->RenameChild(oldFilePath, newFilePath);
    }
    m_map.emplace(newFilePath, node);
    m_map.erase(oldFilePath);
    if (node->IsDirectory()) {
      auto childs = FindChildren(oldFilePath);
      for (auto &child : childs) {
        m_parentToChildrenMap.emplace(newFilePath, std::move(child));
      }
      m_parentToChildrenMap.erase(oldFilePath);
    }
    // m_currentNode = node;
  } else {
    DebugWarning("Node not exist " + FormatPath(oldFilePath));
  }

  return node;
}

// --------------------------------------------------------------------------
void DirectoryTree::Remove(const string &path) {
  if (IsRootDirectory(path)) {
    DebugWarning("Unable to remove root");
    return;
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(path).lock();
  if (!(node && *node)) {
    DebugInfo("No such file or directory, no remove " + FormatPath(path));
    return;
  }

  DebugInfo("Remove node " + FormatPath(path));
  auto parent = node->GetParent();
  if (parent) {
    // if path is a directory, when go out of this function, destructor
    // will recursively delete all its children, as there is no references
    // to the node now.
    parent->Remove(path);
  }
  m_map.erase(path);
  m_parentToChildrenMap.erase(path);

  if (!node->IsDirectory()) {
    node.reset();
    return;
  }

  std::queue<shared_ptr<Node>> deleteNodes;
  for (auto &pair : node->GetChildren()) {
    deleteNodes.push(std::move(pair.second));
  }
  // recursively remove all children references
  while (!deleteNodes.empty()) {
    auto node_ = deleteNodes.front();
    deleteNodes.pop();

    auto path_ = node_->GetFilePath();
    m_map.erase(path_);
    m_parentToChildrenMap.erase(path_);

    if (node->IsDirectory()) {
      for (auto &pair : node_->GetChildren()) {
        deleteNodes.push(std::move(pair.second));
      }
    }
  }
}

// --------------------------------------------------------------------------
shared_ptr<Node> DirectoryTree::HardLink(const string &filePath,
                                         const string &hardlinkPath) {
  // DO not use this for now.
  // HardLink currently shared meta data of target file when create it,
  // Still need to synchronize with target file, to support this we may need
  // to refactory Node to contain a shared_ptr<Entry>.
  DebugInfo("Hard link " + FormatPath(filePath, hardlinkPath));
  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = Find(filePath).lock();
  if (!(node && *node)) {
    DebugWarning("No such file " + FormatPath(filePath));
    return shared_ptr<Node>(nullptr);
  }
  if (node->IsDirectory()) {
    DebugError("Unable to hard link to a directory " +
               FormatPath(filePath, hardlinkPath));
    return shared_ptr<Node>(nullptr);
  }

  auto lnkNode = make_shared<Node>(Entry(node->GetEntry()), node);
  if (!(lnkNode && *lnkNode)) {
    DebugWarning("Fail to hard link " + FormatPath(filePath, hardlinkPath));
    return shared_ptr<Node>(nullptr);
  }
  lnkNode->SetHardLink(true);
  node->Insert(lnkNode);
  node->IncreaseNumLink();
  m_map.emplace(hardlinkPath, lnkNode);
  m_parentToChildrenMap.emplace(node->GetFilePath(), lnkNode);
  // m_currentNode = lnkNode;
  return lnkNode;
}

// --------------------------------------------------------------------------
DirectoryTree::DirectoryTree(time_t mtime, uid_t uid, gid_t gid, mode_t mode) {
  lock_guard<recursive_mutex> lock(m_mutex);
  m_root = make_shared<Node>(
      Entry(ROOT_PATH, 0, mtime, mtime, uid, gid, mode, FileType::Directory));
  // m_currentNode = m_root;
  m_map.emplace(ROOT_PATH, m_root);
}

}  // namespace Data
}  // namespace QS
