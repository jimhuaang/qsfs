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

#include "data/Cache.h"

#include <assert.h>
#include <string.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "data/Directory.h"
#include "data/StreamUtils.h"
#include "filesystem/Configure.h"
#include "filesystem/Drive.h"

namespace QS {

namespace Data {

using QS::Data::Node;
using QS::Data::StreamUtils::GetStreamSize;
using QS::FileSystem::Configure::GetCacheTemporaryDirectory;
using QS::StringUtils::FormatPath;
using QS::StringUtils::PointerAddress;
using QS::Utils::CreateDirectoryIfNotExists;
using QS::Utils::GetBaseName;
using std::deque;
using std::iostream;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

// --------------------------------------------------------------------------
bool Cache::HasFreeSpace(size_t size) const {
  return GetSize() + size <= QS::FileSystem::Configure::GetMaxFileCacheSize();
}

// --------------------------------------------------------------------------
bool Cache::IsLastFileOpen() const{
  if(m_cache.empty()) {
    return false;
  }
  const string &fileName = m_cache.back().first;
  auto &drive = QS::FileSystem::Drive::Instance();
  auto &dirTree = drive.GetDirectoryTree();
  assert(dirTree);
  auto node = dirTree->Find(fileName).lock();
  if(!(node && *node)) {
    return false;
  }
  return node->IsFileOpen();
}

// --------------------------------------------------------------------------
bool Cache::HasFileData(const string &filePath, off_t start,
                        size_t size) const {
  if (!HasFile(filePath)) {
    return false;
  }
  auto it = m_map.find(filePath);
  auto &file = it->second->second;
  return file->HasData(start, size);
}

// --------------------------------------------------------------------------
ContentRangeDeque Cache::GetUnloadedRanges(const string &filePath,
                                           uint64_t fileTotalSize) const {
  ContentRangeDeque ranges;
  if (!HasFile(filePath)) {
    ranges.emplace_back(0, fileTotalSize);
    return ranges;
  }
  auto it = m_map.find(filePath);
  auto &file = it->second->second;

  return file->GetUnloadedRanges(fileTotalSize);
}

// --------------------------------------------------------------------------
bool Cache::HasFile(const string &filePath) const {
  return m_map.find(filePath) != m_map.end();
}

// --------------------------------------------------------------------------
int Cache::GetNumFile() const { return m_map.size(); }

// --------------------------------------------------------------------------
time_t Cache::GetTime(const string &fileId) const {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    auto &file = it->second->second;
    return file->GetTime();
  } else {
    return 0;
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::Find(const string &filePath) {
  auto it = m_map.find(filePath);
  return it != m_map.end() ? it->second : m_cache.end();
}

// --------------------------------------------------------------------------
CacheListIterator Cache::Begin() { return m_cache.begin(); }

// --------------------------------------------------------------------------
CacheListIterator Cache::End() { return m_cache.end(); }

// --------------------------------------------------------------------------
size_t Cache::Read(const string &fileId, off_t offset, size_t len, char *buffer,
                   shared_ptr<Node> node) {
  if (len == 0) {
    return 0;  // do nothing, this case could happen for truncate file to empty
  }

  bool validInput =
      !fileId.empty() && offset >= 0 && len >= 0 && buffer != NULL;
  assert(validInput);
  if (!validInput) {
    DebugError("Try to read cache with invalid input " +
               ToStringLine(fileId, offset, len, buffer));
    return 0;
  }
  if (!(node && *node)) {
    DebugError("Null input node parameter");
    return 0;
  }

  DebugInfo("Read cache [offset:len=" + to_string(offset) + ":" +
            to_string(len) + "] " + FormatPath(fileId));
  memset(buffer, 0, len);  // Clear input buffer.
  size_t sizeBegin = 0;
  auto pos = m_cache.begin();
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    pos = UnguardedMakeFileMostRecentlyUsed(it->second);
    sizeBegin = pos->second->GetSize();
  } else {
    DebugInfo("File not exist in cache. Create new one" +
              FormatPath(node->GetFilePath()));
    pos = UnguardedNewEmptyFile(fileId);
    assert(pos != m_cache.end());
  }

  auto &file = pos->second;
  auto outcome = file->Read(offset, len, &(node->GetEntry()));
  if (outcome.first == 0 || outcome.second.empty()) {
    DebugWarning("Read no bytes from file [offset:len=" + to_string(offset) +
                 ":" + to_string(len) + "] " + FormatPath(node->GetFilePath()));
    return 0;
  }

  size_t addedSize = file->GetSize() - sizeBegin;
  if (addedSize > 0) {
    // Update cache status.
    bool success = Free(addedSize, fileId);
    if (!success) {
      DebugWarning("Cache is full. Unable to free added" +
                   to_string(addedSize) + " bytes when reading file " +
                   FormatPath(fileId));
    }
    m_size += addedSize;
  }

  // Notice outcome pagelist could has more content than required
  auto &pagelist = outcome.second;
  auto &page = pagelist.front();
  pagelist.pop_front();
  if (pagelist.empty()) {  // Only a single page.
    auto sz = std::min(len, outcome.first);
    return page->UnguardedRead(offset, sz, buffer);
  } else {  // Have Multipule pages.
    // read first page
    auto readSize = page->UnguardedRead(offset, buffer);
    page = pagelist.front();
    pagelist.pop_front();
    // read middle pages
    while (!pagelist.empty()) {
      readSize += page->UnguardedRead(buffer + readSize);
      page = pagelist.front();
      pagelist.pop_front();
    }
    // read last page
    auto sz = std::min(outcome.first - readSize, len - readSize);
    readSize += page->UnguardedRead(sz, buffer + readSize);
    return readSize;
  }
}

// --------------------------------------------------------------------------
bool Cache::Write(const string &fileId, off_t offset, size_t len,
                  const char *buffer, time_t mtime) {
  if (len == 0) {
    auto it = m_map.find(fileId);
    if (it != m_map.end()) {
      UnguardedMakeFileMostRecentlyUsed(it->second);
    } else {
      auto pos = UnguardedNewEmptyFile(fileId);
      assert(pos != m_cache.end());
    }
    return true;  // do nothing
  }

  bool validInput =
      !fileId.empty() && offset >= 0 && len >= 0 && buffer != NULL;
  assert(validInput);
  if (!validInput) {
    DebugError("Try to write cache with invalid input " +
               ToStringLine(fileId, offset, len, buffer));
    return false;
  }

  DebugInfo("Write cache [offset:len=" + to_string(offset) + ":" +
            to_string(len) + "] " + FormatPath(fileId));
  auto res = PrepareWrite(fileId, len);
  auto success = res.first;
  if (success) {
    auto file = res.second;
    assert(file != nullptr);
    auto res = (*file)->Write(offset, len, buffer, mtime);
    success = res.first;
    if (success) {
      m_size += res.second;
    }
  }
  return success;
}

// --------------------------------------------------------------------------
bool Cache::Write(const string &fileId, off_t offset, size_t len,
                  shared_ptr<iostream> &&stream, time_t mtime) {
  if (len == 0) {
    auto it = m_map.find(fileId);
    if (it != m_map.end()) {
      UnguardedMakeFileMostRecentlyUsed(it->second);
    } else {
      auto pos = UnguardedNewEmptyFile(fileId);
      assert(pos != m_cache.end());
    }
    return true;  // do nothing
  }

  bool isValidInput = !fileId.empty() && offset >= 0 && stream;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Invalid input [file:offset=" + fileId + ":" +
               to_string(offset) + "]");
    return false;
  }

  size_t streamsize = GetStreamSize(stream);
  assert(len <= streamsize);
  if (!(len <= streamsize)) {
    DebugError(
        "Invalid input, stream buffer size is less than input 'len' parameter. "
        "[file:len=" +
        fileId + ":" + to_string(len) + "]");
    return false;
  }

  DebugInfo("Write cache [offset:len=" + to_string(offset) + ":" +
            to_string(len) + "] " + FormatPath(fileId));
  auto res = PrepareWrite(fileId, len);
  auto success = res.first;
  if (success) {
    auto file = res.second;
    assert(file != nullptr);
    auto res = (*file)->Write(offset, len, std::move(stream), mtime);
    success = res.first;
    if (success) {
      m_size += res.second;
    }
  }
  return success;
}

// --------------------------------------------------------------------------
pair<bool, unique_ptr<File> *> Cache::PrepareWrite(const string &fileId,
                                                   size_t len) {
  bool availableFreeSpace = true;
  if (!HasFreeSpace(len)) {
    availableFreeSpace = Free(len, fileId);

    if (!availableFreeSpace) {
      auto tmpfolder = GetCacheTemporaryDirectory();
      if (!CreateDirectoryIfNotExists(tmpfolder)) {
        DebugError("Unable to mkdir for tmp folder " + FormatPath(tmpfolder));
        return {false, nullptr};
      }
      if (!QS::Utils::IsSafeDiskSpace(tmpfolder, len)) {
        DebugError("No available free space for tmp folder " +
                   FormatPath(tmpfolder));
        return {false, nullptr};
      }
    }
  }

  auto pos = m_cache.begin();
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    pos = UnguardedMakeFileMostRecentlyUsed(it->second);
  } else {
    pos = UnguardedNewEmptyFile(fileId);
    assert(pos != m_cache.end());
  }

  auto &file = pos->second;
  if (!availableFreeSpace) {
    file->SetUseTempFile(true);
  }

  return {true, &file};
}

// --------------------------------------------------------------------------
bool Cache::Free(size_t size, const string &fileUnfreeable) {
  if (size > QS::FileSystem::Configure::GetMaxFileCacheSize()) {
    DebugError("Try to free cache of " + to_string(size) +
               " bytes which surpass the maximum cache size(" +
               to_string(QS::FileSystem::Configure::GetMaxFileCacheSize()) +
               "). Do nothing");
    return false;
  }
  if (HasFreeSpace(size)) {
    DebugInfo("Try to free cache of " + to_string(size) +
              " bytes while free space is still available. Go on");
    return true;
  }

  assert(!m_cache.empty());
  size_t freedSpace = 0;
  while (!HasFreeSpace(size) && !m_cache.empty()) {
    // Discards the least recently used File first, which is put at back.
    // Notice do NOT store a reference of the File supposed to be removed.
    auto fileId = m_cache.back().first;
    if (m_cache.back().second) {
      if(IsLastFileOpen()){
        return false;
      }
      if(fileId == fileUnfreeable){
        return false;
      }
      freedSpace += m_cache.back().second->GetSize();
      m_size -= m_cache.back().second->GetSize();
      m_cache.back().second->Clear();
    } else {
      DebugWarning(
          "The last recently used file (put at the end of cache) is null");
    }
    m_cache.pop_back();
    m_map.erase(fileId);
  }
  DebugInfo("Has freed cache of " + to_string(freedSpace) + " bytes");
  return true;
}

// --------------------------------------------------------------------------
CacheListIterator Cache::Erase(const string &fileId) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    DebugInfo("Erase cache " + FormatPath(fileId));
    return UnguardedErase(it);
  } else {
    DebugInfo("File not exist, no remove " + FormatPath(fileId));
    return m_cache.end();
  }
}

// --------------------------------------------------------------------------
void Cache::Rename(const string &oldFileId, const string &newFileId) {
  if (oldFileId == newFileId) {
    DebugInfo("File exists, no rename " + FormatPath(oldFileId));
    return;
  }

  auto iter = m_map.find(newFileId);
  if (iter != m_map.end()) {
    DebugWarning("File exist, Just remove it from cache " +
                 FormatPath(newFileId));
    UnguardedErase(iter);
  }

  auto it = m_map.find(oldFileId);
  if (it != m_map.end()) {
    it->second->first = newFileId;
    auto pos = UnguardedMakeFileMostRecentlyUsed(it->second);

    m_map.emplace(newFileId, pos);
    m_map.erase(it);
  } else {
    DebugInfo("File not exists, no rename " + FormatPath(oldFileId));
  }
}

// --------------------------------------------------------------------------
void Cache::SetTime(const string &fileId, time_t mtime) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    auto &file = it->second->second;
    file->SetTime(mtime);
  } else {
    DebugWarning("Unable to set time for non existing file " +
                 FormatPath(fileId));
  }
}

// --------------------------------------------------------------------------
void Cache::Resize(const string &fileId, size_t newFileSize, time_t mtime) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    auto &file = it->second->second;
    auto oldFileSize = file->GetSize();
    if (newFileSize == oldFileSize) {
      return;  // do nothing
    } else if (newFileSize > oldFileSize) {
      // fill the hole
      auto holeSize = newFileSize - oldFileSize;
      vector<char> hole(holeSize);  // value initialization with '\0'
      DebugInfo("Fill hole [offset:len=" + to_string(oldFileSize) + ":" +
                to_string(holeSize) + "] " + FormatPath(fileId));
      Write(fileId, oldFileSize, holeSize, &hole[0], mtime);
    } else {
      file->ResizeToSmallerSize(newFileSize);
      file->SetTime(mtime);
    }

    m_size += file->GetSize() - oldFileSize;
    DebugInfoIf(file->GetSize() != newFileSize && !file->UseTempFile(),
                "Try to resize file from size " + to_string(oldFileSize) +
                    " to " + to_string(newFileSize) +
                    ". But now file size is " + to_string(file->GetSize()) +
                    FormatPath(fileId));
    DebugInfoIf(file->UseTempFile(), "temp file used " + FormatPath(fileId));
  } else {
    DebugWarning("Unable to resize non existing file " + FormatPath(fileId));
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedNewEmptyFile(const string &fileId) {
  m_cache.emplace_front(fileId,
                        unique_ptr<File>(new File(GetBaseName(fileId))));
  if (m_cache.begin()->first == fileId) {  // insert to cache sucessfully
    m_map.emplace(fileId, m_cache.begin());
    return m_cache.begin();
  } else {
    DebugError("Fail to create empty file in cache : " + FormatPath(fileId));
    return m_cache.end();
  }
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedErase(
    FileIdToCacheListIteratorMap::iterator pos) {
  auto cachePos = pos->second;
  auto &file = cachePos->second;
  m_size -= file->GetSize();
  file->Clear();
  auto next = m_cache.erase(cachePos);
  m_map.erase(pos);
  return next;
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedMakeFileMostRecentlyUsed(
    CacheListConstIterator pos) {
  m_cache.splice(m_cache.begin(), m_cache, pos);
  // no iterators or references become invalidated, so no need to update m_map.
  return m_cache.begin();
}

}  // namespace Data
}  // namespace QS
