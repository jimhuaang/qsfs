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

#include "data/FileMetaDataManager.h"

#include <memory>
#include <mutex>  // NOLINT

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Data {

using QS::FileSystem::Configure::GetMaxFileMetaDataCount;
using QS::StringUtils::FormatPath;
using std::lock_guard;
using std::recursive_mutex;
using std::string;
using std::to_string;
using std::shared_ptr;
using std::unique_ptr;

static unique_ptr<FileMetaDataManager> instance(nullptr);
static std::once_flag initOnceFlag;

// --------------------------------------------------------------------------
FileMetaDataManager &FileMetaDataManager::Instance() {
  std::call_once(initOnceFlag, [] { instance.reset(new FileMetaDataManager); });
  return *instance.get();
}

// --------------------------------------------------------------------------
MetaDataListConstIterator FileMetaDataManager::Get(
    const std::string &filePath) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return const_cast<FileMetaDataManager *>(this)->Get(filePath);
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Get(const std::string &filePath) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto pos = m_metaDatas.end();
  auto it = m_map.find(filePath);
  if (it != m_map.end()) {
    pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
  } else {
    DebugInfo("File not exist " + FormatPath(filePath));
  }
  return pos;
}

// --------------------------------------------------------------------------
MetaDataListConstIterator FileMetaDataManager::Begin() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.cbegin();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Begin() {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.begin();
}

// --------------------------------------------------------------------------
MetaDataListConstIterator FileMetaDataManager::End() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.cend();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::End() {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.end();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::Has(const std::string &filePath) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return this->Get(filePath) != m_metaDatas.cend();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::HasFreeSpace(size_t needCount) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.size() + needCount < GetMaxFileMetaDataCount();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::AddNoLock(
    shared_ptr<FileMetaData> &&fileMetaData) {
  const auto &filePath = fileMetaData->GetFilePath();
  auto it = m_map.find(filePath);
  if (it == m_map.end()) {  // not exist in manager
    if (!HasFreeSpaceNoLock(1)) {
      auto success = FreeNoLock(1);
      if (!success) return m_metaDatas.end();
    }
    m_metaDatas.emplace_front(filePath, std::move(fileMetaData));
    if (m_metaDatas.begin()->first == filePath) {  // insert sucessfully
      m_map.emplace(filePath, m_metaDatas.begin());
      return m_metaDatas.begin();
    } else {
      DebugWarning("Fail to add file " + FormatPath(filePath));
      return m_metaDatas.end();
    }
  } else {  // exist already, update it
    auto pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
    pos->second = std::move(fileMetaData);
    return pos;
  }
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Add(
    shared_ptr<FileMetaData> &&fileMetaData) {
  lock_guard<recursive_mutex> lock(m_mutex);
  return AddNoLock(std::move(fileMetaData));
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Add(
    std::vector<std::shared_ptr<FileMetaData>> &&fileMetaDatas) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto pos = m_metaDatas.end();
  for (auto &meta : fileMetaDatas) {
    pos = AddNoLock(std::move(meta));
    if (pos == m_metaDatas.end()) break;  // if fail to add an item
  }
  return pos;
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Erase(const std::string &filePath) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto next = m_metaDatas.end();
  auto it = m_map.find(filePath);
  if (it != m_map.end()) {
    next = m_metaDatas.erase(it->second);
    m_map.erase(it);
  } else {
    DebugWarning("File not exist, no remove " + FormatPath(filePath));
  }
  return next;
}

// --------------------------------------------------------------------------
void FileMetaDataManager::Clear() {
  lock_guard<recursive_mutex> lock(m_mutex);
  m_map.clear();
  m_metaDatas.clear();
}

// --------------------------------------------------------------------------
void FileMetaDataManager::Rename(const string &oldFilePath,
                                 const string &newFilePath) {
  if (oldFilePath == newFilePath) {
    // Disable following info
    // DebugInfo("File exist, no rename" + FormatPath(newFilePath) );
    return;
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  if (m_map.find(newFilePath) != m_map.end()) {
    DebugWarning("File exist, no rename " +
                 FormatPath(oldFilePath, newFilePath));
    return;
  }

  auto it = m_map.find(oldFilePath);
  if (it != m_map.end()) {
    it->second->first = newFilePath;
    it->second->second->m_filePath = newFilePath;
    auto pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
    m_map.emplace(newFilePath, pos);
    m_map.erase(it);
  } else {
    DebugWarning("File not exist, no rename " + FormatPath(oldFilePath));
  }
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::UnguardedMakeMetaDataMostRecentlyUsed(
    MetaDataListConstIterator pos) {
  m_metaDatas.splice(m_metaDatas.begin(), m_metaDatas, pos);
  // no iterators or references become invalidated, so no need to update m_map.
  return m_metaDatas.begin();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::HasFreeSpaceNoLock(size_t needCount) const {
  return m_metaDatas.size() + needCount < GetMaxFileMetaDataCount();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::FreeNoLock(size_t needCount) {
  if (needCount > GetMaxFileMetaDataCount()) {
    DebugError("Try to free file meta data manager of " + to_string(needCount) +
               " items which surpass the maximum file meta data count (" +
               to_string(GetMaxFileMetaDataCount()) + "). Do nothing");
    return false;
  }
  if (HasFreeSpaceNoLock(needCount)) {
    DebugInfo("Tre to free file meta data manager of " + to_string(needCount) +
              " items while free space is still availabe. Go on");
    return true;
  }
  while (!HasFreeSpaceNoLock(needCount)) {
    // Discards the least recently used meta first, which is put at back
    assert(!m_metaDatas.empty());
    auto &meta = m_metaDatas.back().second;
    if (meta) {
      meta.reset();
    } else {
      DebugWarning("The last recently used file metadata in manager is null");
    }
    m_metaDatas.pop_back();
  }
  DebugInfo("Has freed file meta data of " + to_string(needCount) + " items");
  return true;
}

// --------------------------------------------------------------------------
FileMetaDataManager::FileMetaDataManager()
    : m_maxCount(GetMaxFileMetaDataCount()) {}

}  // namespace Data
}  // namespace QS
