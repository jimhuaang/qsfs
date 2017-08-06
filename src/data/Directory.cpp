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

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/LogMacros.h"
#include "base/Utils.h"
#include "data/FileMetaDataManager.h"

namespace QS {

namespace Data {

using QS::Data::FileMetaDataManager;
using QS::Utils::EnumHash;
using std::lock_guard;
using std::make_shared;
using std::recursive_mutex;
using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

static const char *const ROOT_PATH = "/";

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
shared_ptr<Node> Node::Find(const string &childFileName) const {
  auto child = m_children.find(childFileName);
  if (child != m_children.end()) {
    return child->second;
  }
  return shared_ptr<Node>(nullptr);
}

// --------------------------------------------------------------------------
const FileNameToNodeUnorderedMap &Node::GetChildren() const {
  return m_children;
}

// --------------------------------------------------------------------------
shared_ptr<Node> Node::Insert(const shared_ptr<Node> &child) {
  if (child) {
    auto res = m_children.emplace(child->GetFileName(), child);
    if (!res.second) {
      DebugInfo(child->GetFileName() +
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
    bool reset = m_children.size() == 1 ? true : false;

    auto it = m_children.find(child->GetFileName());
    if (it != m_children.end()) {
      m_children.erase(it);
      if (reset) m_children.clear();
    } else {
      DebugWarning("Try to remove Node " + child->GetFileName() +
                   " which is not found. Go on");
    }
  } else {
    DebugWarning("Try to remove null Node. Go on")
  }
}

// --------------------------------------------------------------------------
void Node::RenameChild(const string &oldFileName, const string &newFileName) {
  if (oldFileName == newFileName) {
    DebugInfo("New file name is the same as the old one. Go on");
    return;
  }

  if (m_children.find(newFileName) != m_children.end()) {
    DebugWarning("Cannot rename " + oldFileName + " to " + newFileName +
                 " which is already existed. But continue...");
    return;
  }

  auto it = m_children.find(oldFileName);
  if (it != m_children.end()) {
    auto tmp = it->second;
    tmp->SetFileName(newFileName);
    auto hint = m_children.erase(it);
    m_children.emplace_hint(hint, newFileName, tmp);
  } else {
    DebugWarning("Try to rename Node " + oldFileName +
                 " which is not found. Go on");
  }
}

// --------------------------------------------------------------------------
void DirectoryTree::Grow(shared_ptr<FileMetaData> &&fileMeta) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto node = make_shared<Node>(unique_ptr<Entry>(new Entry(fileMeta)));
  m_currentNode = node;

  auto dirName = fileMeta->MyDirName();
  assert(!dirName.empty());
  auto it = m_map.find(dirName);
  if (it != m_map.end()) {
    if (auto parent = it->second.lock()) {
      parent->Insert(node);
      node->SetParent(parent);
    } else {
      DebugInfo("Parent Node of " + fileMeta->GetFileName() +
                " is cleared at the time in directory tree");
    }
  }

  // record parent to children map
  m_parentToChildrenMap.emplace(dirName, node);
  // update
  auto range = m_parentToChildrenMap.equal_range(fileMeta->GetFileName());
  for (auto it = range.first; it != range.second; ++it) {
    if (auto child = it->second.lock()) {
      child->SetParent(node);
      node->Insert(child);
    }
  }

  FileMetaDataManager::Instance().Add(std::move(fileMeta));
}

// --------------------------------------------------------------------------
void DirectoryTree::Grow(vector<shared_ptr<FileMetaData>> &&fileMetas) {
  lock_guard<recursive_mutex> lock(m_mutex);
  for (auto &meta : fileMetas) {
    Grow(std::move(meta));
  }
}

// --------------------------------------------------------------------------
DirectoryTree::DirectoryTree(time_t mtime, uid_t uid, gid_t gid, mode_t mode) {
  lock_guard<recursive_mutex> lock(m_mutex);
  m_root = make_shared<Node>(unique_ptr<Entry>(new Entry(
      ROOT_PATH, 0, mtime, mtime, uid, gid, mode, FileType::Directory)));
  m_currentNode = m_root;
  m_map.emplace(ROOT_PATH, m_root);
}

}  // namespace Data
}  // namespace QS
