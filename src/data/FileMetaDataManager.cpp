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
#include "filesystem/Configure.h"

namespace QS {

namespace Data {

using QS::FileSystem::Configure::GetMaxFileMetaDataEntrys;
using std::lock_guard;
using std::recursive_mutex;
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
    const std::string &fileName) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return const_cast<FileMetaDataManager *>(this)->Get(fileName);
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Get(const std::string &fileName) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto pos = m_metaDatas.end();
  auto it = m_map.find(fileName);
  if (it != m_map.end()) {
    pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
  } else {
    DebugWarning("File (" + fileName +
                 ") is not found in file meta data manager.");
  }
  return pos;
}

// --------------------------------------------------------------------------
MetaDataListConstIterator FileMetaDataManager::Begin() const{
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.cbegin();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Begin(){
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.begin();
}

// --------------------------------------------------------------------------
MetaDataListConstIterator FileMetaDataManager::End() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.cend();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::End(){
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.end();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::Has(const std::string &fileName) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return this->Get(fileName) != m_metaDatas.cend();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::HasFreeSpace(size_t needEntryCount) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_metaDatas.size() + needEntryCount < GetMaxFileMetaDataEntrys();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::AddNoLock(
    shared_ptr<FileMetaData> &&fileMetaData) {
  const auto &fileName = fileMetaData->GetFileName();
  auto it = m_map.find(fileName);
  if (it == m_map.end()) {  // not exist in manager
    if (!HasFreeSpaceNoLock(1)) {
      auto success = FreeNoLock(1);
      if (!success) return m_metaDatas.end();
    }
    m_metaDatas.emplace_front(fileName, std::move(fileMetaData));
    if (m_metaDatas.begin()->first == fileName) {  // insert sucessfully
      m_map.emplace(fileName, m_metaDatas.begin());
      return m_metaDatas.begin();
    } else {
      DebugError("Fail to add meta data into manager for file " + fileName);
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
    if (pos == m_metaDatas.end()) break;  // if fail to add an entry
  }
  return pos;
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Erase(const std::string &fileName) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto next = m_metaDatas.end();
  auto it = m_map.find(fileName);
  if (it != m_map.end()) {
    next = m_metaDatas.erase(it->second);
    m_map.erase(it);
  } else {
    DebugWarning("Try to remove non-existing meta data from manager for file " +
                 fileName);
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
MetaDataListIterator FileMetaDataManager::UnguardedMakeMetaDataMostRecentlyUsed(
    MetaDataListConstIterator pos) {
  m_metaDatas.splice(m_metaDatas.begin(), m_metaDatas, pos);
  // NO iterators or references become invalidated, so no need to update m_map.
  return m_metaDatas.begin();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::HasFreeSpaceNoLock(size_t needEntryCount) const {
  return m_metaDatas.size() + needEntryCount < GetMaxFileMetaDataEntrys();
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::FreeNoLock(size_t needEntryCount) {
  if (needEntryCount > GetMaxFileMetaDataEntrys()) {
    DebugError(
        "Try to free file meta data manager of " + to_string(needEntryCount) +
        " entrys which surpass the maximum file meta data entry count (" +
        to_string(GetMaxFileMetaDataEntrys()) + "). Do nothing");
    return false;
  }
  if (HasFreeSpaceNoLock(needEntryCount)) {
    DebugInfo("Tre to free file meta data manager of " +
              to_string(needEntryCount) +
              " entrys while free space is still availabe. Go on");
    return true;
  }
  while (!HasFreeSpaceNoLock(needEntryCount)) {
    // Discards the least recently used entry first, which is put at back
    assert(!m_metaDatas.empty());
    auto &meta = m_metaDatas.back().second;
    if (meta) {
      meta.reset();
    } else {
      DebugWarning("The last recently used file metadata in manager is null");
    }
    m_metaDatas.pop_back();
  }
  DebugInfo("Has freed file meta data of " + to_string(needEntryCount) +
            " entrys");
  return true;
}

// --------------------------------------------------------------------------
FileMetaDataManager::FileMetaDataManager()
    : m_maxEntrys(GetMaxFileMetaDataEntrys()) {}

}  // namespace Data
}  // namespace QS
