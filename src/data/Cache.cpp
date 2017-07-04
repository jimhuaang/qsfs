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

#include <algorithm>
#include <iterator>

#include "base/FileSystem.h"
#include "base/LogMacros.h"
#include "base/Utils.h"
#include "client/Configuration.h"

namespace QS {

namespace Data {

using QS::Client::GetMaxCacheSize;
using QS::FileSystem::Node;
using QS::FileSystem::Entry;
using QS::Utils::PointerAddress;
using std::lock_guard;
using std::make_shared;
using std::min;
using std::pair;
using std::recursive_mutex;
using std::reverse_iterator;
using std::list;
using std::string;
using std::to_string;
using std::shared_ptr;
using std::unique_ptr;

namespace {
bool IsValidInput(const string &fileId, off_t offset, char *buffer,
                  size_t len) {
  return !fileId.empty() && offset >= 0 && buffer != NULL && len > 0;
}

bool IsValidInput(off_t offset, char *buffer, size_t len) {
  return offset >= 0 && buffer != NULL && len > 0;
}

// TODO(jim): convert fileId to file name
string ToStringLine(const string &fileId, off_t offset, char *buffer,
                    size_t len) {
  return "[fileId:offset:buffer:size] = [" + fileId + ":" + to_string(offset) +
         ":" + PointerAddress(buffer) + ":" + to_string(len) + "]";
}

string ToStringLine(off_t offset, char *buffer, size_t len) {
  return "[offset:buffer:size] = [" + to_string(offset) + ":" +
         PointerAddress(buffer) + ":" + to_string(len) + "]";
}

string ToStringLine(off_t offset, size_t size) {
  return "[offset:size] = [" + to_string(offset) + ":" + to_string(size) + "]";
}
}

// +------------------------------------------------------------------------+
// |                  Page Member Functions                                 |
// +------------------------------------------------------------------------+
File::Page::Page(off_t offset, char *buffer, size_t len)
    : m_offset(offset), m_size(len), m_memory(std::stringstream()) {
  // TODO(jim): consider without checking
  bool isValidInput = IsValidInput(offset, buffer, len);
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, buffer, len) + ".");
    return;
  }

  // TODO(jim) : catch exceptions, no throw excepitons
  m_memory.write(buffer, len);
  DebugErrorIf(
      m_memory.fail(),
      "Fail to new a page from " + ToStringLine(offset, buffer, len) + ".");
}

// --------------------------------------------------------------------------
bool File::Page::Refresh(off_t offset, char *buffer, size_t len) {
  // TODO(jim): consider without checking
  bool isValidInput = offset >= m_offset && buffer != NULL && len > 0;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to refresh page(" + ToStringLine(m_offset, m_size) +
               ") with invalid input " + ToStringLine(offset, buffer, len) +
               ".");
    return false;
  }
  m_memory.seekp(offset - m_offset, std::ios_base::cur);
  m_memory.write(buffer, len);

  auto success = m_memory.good();
  DebugErrorIf(!success,
               "Fail to refresh page(" + ToStringLine(m_offset, m_size) +
                   ") with input " + ToStringLine(offset, buffer, len) + ".");
  return success;
}

// --------------------------------------------------------------------------
void File::Page::Resize(size_t smallerSize) {
  // Do a lazy resize
  // 1. Change size to 'samllerSize'.
  // 2. Set output position indicator to 'samllerSize'.
  assert(0 <= smallerSize && smallerSize <= m_size);
  m_size = smallerSize;
  m_memory.seekp(smallerSize);
}

// --------------------------------------------------------------------------
size_t File::Page::Read(off_t offset, char *buffer, size_t len) {
  bool isValidInput = (offset >= m_offset && buffer != NULL && len > 0 &&
                       len <= static_cast<size_t>(Next() - offset));
  assert(isValidInput);
  DebugErrorIf(!isValidInput,
               "Try to read page (" + ToStringLine(m_offset, m_size) +
                   ") with invalid input " + ToStringLine(offset, buffer, len) +
                   ".");
  return UnguardedRead(offset, buffer, len);
}

// --------------------------------------------------------------------------
size_t File::Page::UnguardedRead(off_t offset, char *buffer, size_t len) {
  m_memory.seekg(offset - m_offset, std::ios_base::beg);
  m_memory.read(buffer, len);
  DebugErrorIf(!m_memory.good(),
               "Fail to read page(" + ToStringLine(m_offset, m_size) +
                   ") with input " + ToStringLine(offset, buffer, len) + ".");
  return len;
}

// --------------------------------------------------------------------------

// +------------------------------------------------------------------------+
// |                  File Member Functions                                 |
// +------------------------------------------------------------------------+
File::ReadOutcome File::Read(off_t offset, size_t len,
                             unique_ptr<Entry> &entry) {
  assert(offset >= 0 && len > 0);
  size_t outcomeSize = 0;
  list<shared_ptr<Page>> outcomePages;

  auto mtime = entry->GetMTime();
  if (mtime > 0) {
    // File is just created, update mtime.
    if (m_mtime == 0) {
      SetTime(mtime);
    }
    // Detected modification in the file, clear file.
    else if (mtime > m_mtime) {
      Clear();
      entry->SetFileSize(0);
    }
  }

  if (entry->GetFileSize() > 0) {
    auto len_ = static_cast<size_t>(entry->GetFileSize() - offset);
    if (len_ < len) {
      len = len_;
      DebugWarning("Try to read file([fileId:size] = [" + entry->GetFileId() +
                   ":" + to_string(entry->GetFileSize()) + "]) with input " +
                   ToStringLine(offset, len) + " which surpass the file size.");
    }
  }

  { lock_guard<recursive_mutex> lock(m_mutex); }

  return {outcomeSize, outcomePages};
}

// --------------------------------------------------------------------------
bool File::Write(off_t offset, char *buffer, size_t len, time_t mtime) {
  auto AddPageAndUpdateTime = [this, mtime](off_t offset, char *buffer,
                                            size_t len) -> bool {
    auto res = this->UnguardedAddPage(offset, buffer, len);
    if (res.second) this->SetTime(mtime);
    return res.second;
  };

  {
    lock_guard<recursive_mutex> lock(m_mutex);
    bool isValidInput = IsValidInput(offset, buffer, len);
    assert(isValidInput);
    if (!isValidInput) {
      DebugError("Fail to write file with invalid input " +
                 ToStringLine(offset, buffer, len) + ".");
      return false;
    }

    // If pages is empty or given input is ahead of or behind of
    // all current pages.
    if (m_pages.empty() ||
        offset + static_cast<off_t>(len) <= Front()->m_offset ||
        offset >= Back()->Next()) {
      return AddPageAndUpdateTime(offset, buffer, len);
    }

    auto it1 = LowerBoundPage(offset);
    auto it2 = LowerBoundPage(offset + len);
    // Move backward it1 to pointing to the page which is not ahead of 'offset'.
    reverse_iterator<PageSetConstIterator> reverseIt(it1);
    it1 = (reverseIt == m_pages.crend() || (*reverseIt)->Next() <= offset)
              ? it1
              : (++reverseIt).base();

    auto offset_ = offset;
    size_t start_ = 0;
    size_t len_ = len;
    // For pages which are not ahead of 'offset' but ahead of 'offset + len'.
    while (it1 != it2) {
      if (len_ <= 0) break;
      auto &page = *it1;
      // Insert new page for bytes not present
      if (offset_ < page->m_offset) {
        auto lenNewPage = page->m_offset - offset_;
        auto res = UnguardedAddPage(offset_, buffer + start_, lenNewPage);
        if (!res.second) return false;

        offset_ = page->m_offset;
        start_ += lenNewPage;
        len_ -= lenNewPage;
      }
      // Refresh the overlapped page's content
      else {
        if (len_ <= static_cast<size_t>(page->Next() - offset_)) {
          SetTime(mtime);
          return page->Refresh(offset_, buffer + start_, len_);
        } else {
          auto refresh = page->Refresh(buffer + start_);
          if (!refresh) return false;

          offset_ = page->Next();
          start_ += page->m_size;
          len_ -= page->m_size;
          ++it1;
        }
      }
    }  // end of while

    // Insert new page for bytes not present
    if (len_ > 0) {
      auto res = UnguardedAddPage(offset_, buffer + start_, len_);
      if (!res.second) return false;
    }

    SetTime(mtime);
  }  // end of lock_guard
  return true;
}

// --------------------------------------------------------------------------
void File::Resize(size_t smallerSize) {
  auto curSize = GetSize();
  if (smallerSize >= curSize) {
    DebugWarning("File size: " + to_string(curSize) +
                 ", target size: " + to_string(smallerSize) +
                 ". Try to resize File to a larger or same size. Go on.");
    return;
  }

  off_t offset = static_cast<off_t>(smallerSize - 1);
  {
    lock_guard<recursive_mutex> lock(m_mutex);

    // Erase pages behind offset
    auto it = UpperBoundPage(offset);
    if (it != m_pages.end()) {
      for (auto page = it; page != m_pages.end(); ++page) {
        m_size -= (*page)->m_size;
      }
      m_pages.erase(it, m_pages.end());
    }

    if (!m_pages.empty()) {
      auto &lastPage = Back();
      if (offset == lastPage->Stop()) {
        // Go on
        return;
      } else if (lastPage->m_offset <= offset && offset < lastPage->Stop()) {
        auto newSize = smallerSize - lastPage->m_offset;
        m_size -= lastPage->m_size - newSize;
        // Do a lazy remove for last page.
        lastPage->Resize(newSize);
      } else {
        DebugError("After erased pages behind offset " + to_string(offset) +
                   " , last page now with " +
                   ToStringLine(lastPage->m_offset, lastPage->m_size) +
                   ". Should not happen, but go on.");
      }
    } else {
      DebugError("File becomes empty after erase pages behind offset of " +
                 to_string(offset) + ". Should not happen, but go on.");
    }
  }
}

// --------------------------------------------------------------------------
void File::Clear() {
  {
    lock_guard<recursive_mutex> lock(m_mutex);
    m_pages.clear();
  }
  m_mtime.store(time(NULL));
  m_size.store(0);
}

// --------------------------------------------------------------------------
File::PageSetConstIterator File::LowerBoundPage(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset);
  return m_pages.lower_bound(tmpPage);
}

// --------------------------------------------------------------------------
File::PageSetConstIterator File::UpperBoundPage(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset);
  return m_pages.upper_bound(tmpPage);
}

// --------------------------------------------------------------------------
pair<File::PageSetConstIterator, bool> File::UnguardedAddPage(off_t offset,
                                                              char *buffer,
                                                              size_t len) {
  auto res = m_pages.emplace(new Page(offset, buffer, len));
  if (res.second) {
    m_size += len;
  } else {
    DebugError("Fail to new a page from a block of character with " +
               ToStringLine(offset, buffer, len) + ".");
  }
  return res;
}

// +------------------------------------------------------------------------+
// |                  Cache Member Functions                                |
// +------------------------------------------------------------------------+
size_t Cache::Read(const string &fileId, off_t offset, char *buffer, size_t len,
                   shared_ptr<Node> node)

{
  bool validInput = IsValidInput(fileId, offset, buffer, len);
  assert(validInput);
  if (!validInput) {
    DebugError("Try to read cache with invalid input " +
               ToStringLine(fileId, offset, buffer, len) + ".");
    return 0;
  }
  memset(buffer, 0, len);  // clear input buffer

  bool fileIsJustCreated = false;
  auto pos = m_cache.begin();
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    pos = UnguardedMakeFileMostRecentlyUsed(it->second);
  } else {
    DebugInfo("File(" + node->GetFileName() +
              ") is not found in cache. Creating new one.");
    fileIsJustCreated = true;
    pos = UnguardedNewEmptyFile(fileId);
    assert(pos != m_cache.end());
  }

  auto &file = pos->second;
  auto outcome = file->Read(offset, len, node->GetEntry());
  if (outcome.first == 0 || outcome.second.empty()) {
    DebugInfo("Read no bytes from file(" + node->GetFileName() + ").");
    return 0;
  }
  if (fileIsJustCreated) {
    // Update cache status
    Free(file->GetSize());
    m_size += file->GetSize();
  }

  auto &pagelist = outcome.second;
  auto &page = pagelist.front();
  pagelist.pop_front();
  // Only a single page.
  if (pagelist.empty()) {
    page->UnguardedRead(offset, buffer, outcome.first);
  }
  // Have Multipule pages.
  else {
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
    page->UnguardedRead(buffer + readSize, outcome.first - readSize);
  }

  return outcome.first;
}

// --------------------------------------------------------------------------
bool Cache::Write(const string &fileId, off_t offset, char *buffer, size_t len,
                  time_t mtime) {
  bool validInput = IsValidInput(fileId, offset, buffer, len);
  assert(validInput);
  if (!validInput) {
    DebugError("Try to write cache with invalid input " +
               ToStringLine(fileId, offset, buffer, len) + ".");
    return false;
  }

  if (!HasFreeSpace(len)) {
    auto success = Free(len);
    if (!success) return false;
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
  auto success = file->Write(offset, buffer, len, mtime);
  if (success) {
    m_size += len;
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------
bool Cache::Free(size_t size) {
  if (size > GetMaxCacheSize()) {
    DebugError("Try to free cache of " + to_string(size) +
               " bytes which surpass the maximum cache size(" +
               to_string(GetMaxCacheSize()) + ").");
    return false;
  }
  if (HasFreeSpace(size)) {
    DebugInfo("Try to free cache of " + to_string(size) +
              " bytes while free space is still available. Go on.");
    return true;
  }

  size_t freedSpace = 0;
  while (!HasFreeSpace(size)) {
    // Discards the least recently used File first,
    // which is put at back.
    auto &file = m_cache.back().second;
    freedSpace += file->GetSize();
    m_size -= file->GetSize();
    file->Clear();
  }
  DebugInfo("Has freed cache of " + to_string(freedSpace) + " bytes.");
  return true;
}

// --------------------------------------------------------------------------
void Cache::Erase(const string &fileId) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    UnguardedErase(it);
  } else {
    DebugWarning("Try to remove file " + fileId +
                 " which is not found. Go on.");
  }
}

// --------------------------------------------------------------------------
void Cache::Rename(const string &oldFileId, const string &newFileId) {
  if (oldFileId == newFileId) {
    DebugInfo("The new file id is the same as the old one. Go on.");
    return;
  }

  auto iter = m_map.find(newFileId);
  if (iter != m_map.end()) {
    DebugWarning(
        "New file id: + " + newFileId +
        " is already existed in the cache. Just remove it from cache.");
    UnguardedErase(iter);
  }

  auto it = m_map.find(oldFileId);
  if (it != m_map.end()) {
    it->second->first = newFileId;
    auto pos = UnguardedMakeFileMostRecentlyUsed(it->second);

    m_map.emplace(newFileId, pos);
    m_map.erase(it);
  } else {
    DebugWarning("Try to rename file id " + oldFileId +
                 " which is not found. Go on.")
  }
}

// --------------------------------------------------------------------------
void Cache::SetTime(const string &fileId, time_t mtime) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    auto &file = it->second->second;
    file->SetTime(mtime);
  } else {
    DebugWarning("Try to set time for file " + fileId +
                 " which is not found. Go on.");
  }
}

// --------------------------------------------------------------------------
void Cache::Resize(const string &fileId, size_t newSize) {
  auto it = m_map.find(fileId);
  if (it != m_map.end()) {
    auto &file = it->second->second;
    auto oldSize = file->GetSize();
    file->Resize(newSize);
    m_size += file->GetSize() - oldSize;

    DebugInfoIf(file->GetSize() != newSize,
                "Try to resize file " + fileId + " from size " +
                    to_string(oldSize) + " to " + to_string(newSize) +
                    ". But now file size is " + to_string(file->GetSize()) +
                    ".");
  } else {
    DebugWarning("Try to resize file " + fileId +
                 " which is not found. Go on.");
  }
}

// --------------------------------------------------------------------------
bool Cache::HasFreeSpace(size_t size) const {
  return GetSize() + size < QS::Client::GetMaxCacheSize();
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedNewEmptyFile(const string &fileId) {
  m_cache.emplace_front(fileId, unique_ptr<File>(new File));
  if (m_cache.begin()->first == fileId) {
    m_map.emplace(fileId, m_cache.begin());
    return m_cache.begin();
  } else {
    DebugError("Fail to new an empty file with fileId : " + fileId + ".");
    return m_cache.end();
  }
}

// --------------------------------------------------------------------------
void Cache::UnguardedErase(FileIdToCacheListIteratorMap::iterator pos) {
  m_size -= pos->second->second->GetSize();
  m_cache.erase(pos->second);
  m_map.erase(pos);
}

// --------------------------------------------------------------------------
CacheListIterator Cache::UnguardedMakeFileMostRecentlyUsed(
    CacheListConstIterator pos) {
  m_cache.splice(m_cache.begin(), m_cache, pos);
  return m_cache.begin();
}

}  // namespace Data
}  // namespace QS
