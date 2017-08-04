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

using std::lock_guard;
using std::recursive_mutex;
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
    const std::string &fileId) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return const_cast<FileMetaDataManager *>(this)->Get(fileId);
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Get(const std::string &fileId) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto pos = m_metaDatas.end();
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
  } else {
    DebugWarning("File (" + fileId +
                 ") is not found in file meta data manager.");
  }
  return pos;
}

// --------------------------------------------------------------------------
bool FileMetaDataManager::Has(const std::string &fileId) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return this->Get(fileId) != m_metaDatas.cend();
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Add(
    std::unique_ptr<FileMetaData> fileMetaData) {
  lock_guard<recursive_mutex> lock(m_mutex);
  const auto &fileId = fileMetaData->m_fileId;
  auto it = m_map.find(fileId);
  if (it == m_map.end()) {  // not exist in manager
    m_metaDatas.emplace_front(fileId, std::move(fileMetaData));
    if (m_metaDatas.begin()->first == fileId) {  // insert sucessfully
      m_map.emplace(fileId, m_metaDatas.begin());
      return m_metaDatas.begin();
    } else {
      DebugError("Fail to add meta data into manager for file " + fileId);
      return m_metaDatas.end();
    }
  } else {  // exist already, update it
    auto pos = UnguardedMakeMetaDataMostRecentlyUsed(it->second);
    pos->second = std::move(fileMetaData);
    return pos;
  }
}

// --------------------------------------------------------------------------
MetaDataListIterator FileMetaDataManager::Erase(const std::string &fileId) {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto next = m_metaDatas.end();
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    next = m_metaDatas.erase(it->second);
    m_map.erase(it);
  } else {
    DebugWarning("Try to remove non-existing meta data from manager for file " +
                 fileId);
  }
  return next;
}

// --------------------------------------------------------------------------
void FileMetaDataManager::Clear() {
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
FileMetaDataManager::FileMetaDataManager()
    : m_maxEntrys(QS::FileSystem::Configure::GetMaxFileMetaDataEntrys()) {}

}  // namespace Data
}  // namespace QS
