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

#include <base/FileSystem.h>

#include <cassert>

#include <string>
#include <unordered_map>

#include <base/Utils.h>

namespace QS {

namespace FileSystem {

using QS::Utils::EnumHash;
using std::string;
using std::shared_ptr;
using std::unordered_map;

const string &GetFileTypeName(FileType fileType) {
  static unordered_map<FileType, string, EnumHash> fileTypeNames = {
      {FileType::None, "None"},
      {FileType::File, "File"},
      {FileType::Directory, "Directory"}};
  return fileTypeNames[fileType];
}

/**================================================== *
 * ==========       Node Functions         ========== *
 * ================================================== */
bool Node::IsEmpty() const { return m_children.empty(); }

shared_ptr<Node> Node::Find(const string &fileName) const {
  auto child = m_children.find(fileName);
  if (child != m_children.end()) {
    return child->second;
  }
  return shared_ptr<Node>(nullptr);
}

const StlFileNameToNodeMap &Node::GetChildren() const { return m_children; }

shared_ptr<Node> Node::Insert(shared_ptr<Node> child) {
  if (child) {
    m_children.emplace(child->m_fileName, child);
  }
  return child;
}

void Node::Remove(shared_ptr<Node> child) {
  if (child) {
    bool reset = m_children.size() == 1 ? true : false;

    auto it = m_children.find(child->m_fileName);
    if (it != m_children.end()) {
      m_children.erase(it);
      if (reset) m_children.clear();
    }
  }
}

void Node::RenameChild(const string &oldFileName, const string &newFileName) {
  auto it = m_children.find(oldFileName);

  if (it != m_children.end()) {
    auto tmp = it->second;
    tmp->m_fileName = newFileName;
    auto hint = m_children.erase(it);
    m_children.emplace_hint(hint, newFileName, tmp);
  }
}

}  // namespace FileSystem
}  // namespace QS
