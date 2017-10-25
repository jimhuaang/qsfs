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

#include "data/File.h"

#include <assert.h>
#include <stdio.h>  // for pclose

#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "configure/Default.h"
#include "data/IOStream.h"

namespace QS {

namespace Data {

using QS::Configure::Default::GetCacheTemporaryDirectory;
using QS::StringUtils::PointerAddress;
using QS::Utils::FileExists;
using QS::Utils::RemoveFileIfExists;
using QS::Utils::RemoveFileIfExistsNoLog;
using std::iostream;
using std::list;
using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::pair;
using std::recursive_mutex;
using std::reverse_iterator;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::tuple;
using std::unique_ptr;
using std::vector;

namespace {

// Build a tmp file absolute path
//
// @param  : file base name
// @return : string
//
string BuildTempFilePath(const string &basename) {
  string qsfsTmpDir = GetCacheTemporaryDirectory();
  return qsfsTmpDir + basename;
}

// --------------------------------------------------------------------------
string PrintFileName(const string &file) { return "[file=" + file + "]"; }

}  // namespace

// --------------------------------------------------------------------------
File::~File() {
  // As pages using tmp file will reference to the same tmp file, so File should
  // manage the life cycle of the tmp file.
  RemoveTempFileFromDiskIfExists(true);  // log on
}

// --------------------------------------------------------------------------
string File::AskTempFilePath() const { return BuildTempFilePath(m_baseName); }

// --------------------------------------------------------------------------
pair<PageSetConstIterator, PageSetConstIterator>
File::ConsecutivePageRangeAtFront() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  if (m_pages.empty()) {
    return {m_pages.begin(), m_pages.begin()};
  }
  auto cur = m_pages.begin();
  auto next = m_pages.begin();
  while (++next != m_pages.end()) {
    if ((*cur)->Next() < (*next)->Offset()) {
      break;
    }
    ++cur;
  }
  return {m_pages.begin(), next};
}

// --------------------------------------------------------------------------
bool File::HasData(off_t start, size_t size) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  auto stop = static_cast<off_t>(start + size);
  auto range = IntesectingRange(start, stop);
  if (range.first == range.second) {
    if (range.first == m_pages.end()) {
      if (size == 0 && start <= static_cast<off_t>(m_size)) {
        return true;
      }
    }
    return false;
  }
  // find the consecutive pages at front [beg to cur]
  auto beg = range.first;
  auto cur = beg;
  auto next = beg;
  while (++next != range.second) {
    if ((*cur)->Next() < (*next)->Offset()) {
      break;
    }
    ++cur;
  }

  return ((*beg)->Offset() <= start && stop <= (*cur)->Next());
}

// --------------------------------------------------------------------------
ContentRangeDeque File::GetUnloadedRanges(off_t start, size_t size) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  ContentRangeDeque ranges;
  if (size == 0 || m_size == 0 || m_pages.empty()) {
    return ranges;
  }

  off_t stop = static_cast<off_t>(start + size);
  auto range = IntesectingRange(start, stop);

  if (range.first == range.second) {
    return ranges;
  }

  auto cur = range.first;
  auto next = range.first;
  while (++next != range.second) {
    if ((*cur)->Next() < (*next)->Offset()) {
      if ((*next)->Offset() > static_cast<off_t>(size)) {
        break;
      }
      off_t off = (*cur)->Next();
      size_t size = static_cast<size_t>((*next)->Offset() - off);
      ranges.emplace_back(off, size);
    }
    ++cur;
  }

  if ((*cur)->Next() < stop) {
    off_t off = (*cur)->Next();
    size_t size = static_cast<size_t>(stop - off);
    ranges.emplace_back(off, size);
  }

  return ranges;
}

// --------------------------------------------------------------------------
PageSetConstIterator File::BeginPage() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_pages.begin();
}

// --------------------------------------------------------------------------
PageSetConstIterator File::EndPage() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_pages.end();
}

// --------------------------------------------------------------------------
size_t File::GetNumPages() const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return m_pages.size();
}

// --------------------------------------------------------------------------
tuple<size_t, list<shared_ptr<Page>>, ContentRangeDeque> File::Read(
    off_t offset, size_t len, time_t mtimeSince) {
  // Cache already check input.
  // bool isValidInput = offset >= 0 && len > 0 && entry != nullptr && (*entry);
  // assert(isValidInput);
  // if (!isValidInput) {
  //   DebugError("Fail to read file with invalid input " +
  //              ToStringLine(offset, len));
  //   return {0, list<shared_ptr<Page>>()};
  // }

  ContentRangeDeque unloadedRanges;
  auto AddUnloadedPages = [&unloadedRanges](off_t offset, size_t len) {
    unloadedRanges.emplace_back(offset, len);
  };

  size_t outcomeSize = 0;
  list<shared_ptr<Page>> outcomePages;

  if (len == 0) {
    AddUnloadedPages(offset, len);
    return make_tuple(outcomeSize, outcomePages, unloadedRanges);
  }

  {
    lock_guard<recursive_mutex> lock(m_mutex);

    if (mtimeSince > 0) {
      // File is just created, update mtime.
      if (m_mtime == 0) {
        SetTime(mtimeSince);
      } else if (mtimeSince > m_mtime) {
        // Detected modification in the file
        AddUnloadedPages(offset, len);
        return make_tuple(outcomeSize, outcomePages, unloadedRanges);
      }
    }

    // If pages is empty.
    if (m_pages.empty()) {
      AddUnloadedPages(offset, len);
      return make_tuple(outcomeSize, outcomePages, unloadedRanges);
    }

    auto range = IntesectingRange(offset, offset + len);
    auto it1 = range.first;
    auto it2 = range.second;
    auto offset_ = offset;
    size_t len_ = len;
    // For pages which are not completely ahead of 'offset'
    // but ahead of 'offset + len'.
    while (it1 != it2) {
      if (len_ <= 0) break;
      auto &page = *it1;
      if (offset_ <
          page->m_offset) {  // Add unloaded page for bytes not present.
        auto lenNewPage = page->m_offset - offset_;
        AddUnloadedPages(offset_, lenNewPage);
        offset_ = page->m_offset;
        len_ -= lenNewPage;
      } else {  // Collect existing pages.
        if (len_ <= static_cast<size_t>(page->Next() - offset_)) {
          outcomePages.emplace_back(page);
          outcomeSize += page->m_size;
          return make_tuple(outcomeSize, outcomePages, unloadedRanges);
        } else {
          outcomePages.emplace_back(page);
          outcomeSize += page->m_size;

          len_ -= page->Next() - offset_;
          offset_ = page->Next();
          ++it1;
        }
      }
    }  // end of while
    // Add unloaded range for bytes not present.
    if (len_ > 0) {
      AddUnloadedPages(offset_, len_);
    }
  }  // end of lock_guard

  return make_tuple(outcomeSize, outcomePages, unloadedRanges);
}

// --------------------------------------------------------------------------
tuple<bool, size_t, size_t> File::Write(off_t offset, size_t len,
                                        const char *buffer, time_t mtime) {
  // Cache has checked input.
  // bool isValidInput = = offset >= 0 && len > 0 &&  buffer != NULL;
  // assert(isValidInput);
  // if (!isValidInput) {
  //   DebugError("Fail to write file with invalid input " +
  //              ToStringLine(offset, len, buffer));
  //   return false;
  // }

  size_t addedSizeInCache = 0;
  size_t addedSize = 0;
  auto AddPageAndUpdateTime = [this, mtime, &addedSizeInCache, &addedSize](
      off_t offset, size_t len, const char *buffer) -> bool {
    auto res = this->UnguardedAddPage(offset, len, buffer);
    if (std::get<1>(res)) {
      if (mtime > m_mtime) {
        this->SetTime(mtime);
      }
      addedSizeInCache += std::get<2>(res);
      addedSize += std::get<3>(res);
    }
    return std::get<1>(res);
  };

  lock_guard<recursive_mutex> lock(m_mutex);
  // If pages is empty.
  if (m_pages.empty()) {
    return make_tuple(AddPageAndUpdateTime(offset, len, buffer),
                      addedSizeInCache, addedSize);
  }

  bool success = true;
  auto range = IntesectingRange(offset, offset + len);
  auto it1 = range.first;
  auto it2 = range.second;
  auto offset_ = offset;
  size_t start_ = 0;
  size_t len_ = len;
  // For pages which are not completely ahead of 'offset'
  // but ahead of 'offset + len'.
  while (it1 != it2) {
    if (len_ <= 0) break;
    auto &page = *it1;
    if (offset_ < page->m_offset) {  // Insert new page for bytes not present.
      auto lenNewPage = page->m_offset - offset_;
      auto res = UnguardedAddPage(offset_, lenNewPage, buffer + start_);
      if (!std::get<1>(res)) {
        success = false;
        return make_tuple(false, addedSizeInCache, addedSize);
      } else {
        addedSizeInCache += std::get<2>(res);
        addedSize += std::get<3>(res);
      }

      offset_ = page->m_offset;
      start_ += lenNewPage;
      len_ -= lenNewPage;
    } else {  // Refresh the overlapped page's content.
      if (len_ <= static_cast<size_t>(page->Next() - offset_)) {
        if (mtime >= m_mtime) {
          SetTime(mtime);
          // refresh parital content of page
          return make_tuple(page->Refresh(offset_, len_, buffer + start_),
                            addedSizeInCache, addedSize);
        } else {
          // do nothing
          return make_tuple(true, addedSizeInCache, addedSize);
        }
      } else {
        if (mtime >= m_mtime) {
          // refresh entire page
          auto refresh = page->Refresh(buffer + start_);
          if (!refresh) {
            success = false;
            return make_tuple(false, addedSizeInCache, addedSize);
          }

          SetTime(mtime);
        }
        offset_ = page->Next();
        start_ += page->m_size;
        len_ -= page->m_size;
        ++it1;
      }
    }
  }  // end of while
  // Insert new page for bytes not present.
  if (len_ > 0) {
    auto res = UnguardedAddPage(offset_, len_, buffer + start_);
    if (std::get<1>(res)) {
      success = true;

      if (mtime > m_mtime) {
        this->SetTime(mtime);
      }
      addedSizeInCache += std::get<2>(res);
      addedSize += std::get<3>(res);
    } else {
      success = false;
    }
  }

  return make_tuple(success, addedSizeInCache, addedSize);
}

// --------------------------------------------------------------------------
tuple<bool, size_t, size_t> File::Write(off_t offset, size_t len,
                                        shared_ptr<iostream> &&stream,
                                        time_t mtime) {
  auto AddPageAndUpdateTime = [this, mtime](
      off_t offset, size_t len,
      shared_ptr<iostream> &&stream) -> tuple<bool, size_t, size_t> {
    auto res = this->UnguardedAddPage(offset, len, std::move(stream));
    if (std::get<1>(res) && mtime > m_mtime) {
      this->SetTime(mtime);
    }
    return make_tuple(std::get<1>(res), std::get<2>(res), std::get<3>(res));
  };

  lock_guard<recursive_mutex> lock(m_mutex);
  if (m_pages.empty()) {
    return AddPageAndUpdateTime(offset, len, std::move(stream));
  } else {
    auto it = LowerBoundPage(offset);
    auto &page = *it;
    if (it == m_pages.end()) {
      return AddPageAndUpdateTime(offset, len, std::move(stream));
    } else if (page->Offset() == offset && page->Size() == len) {
      if (mtime >= m_mtime) {
        // replace old stream
        page->SetStream(std::move(stream));
        SetTime(mtime);
      }
      return make_tuple(true, 0, 0);
    } else {
      auto buf = unique_ptr<vector<char>>(new vector<char>(len));
      stream->seekg(0, std::ios_base::beg);
      stream->read(&(*buf)[0], len);

      return Write(offset, len, &(*buf)[0], mtime);
    }
  }
}

// --------------------------------------------------------------------------
void File::ResizeToSmallerSize(size_t smallerSize) {
  auto curSize = GetSize();
  if (smallerSize == curSize) {
    return;
  }
  if (smallerSize > curSize) {
    DebugWarning("File size: " + to_string(curSize) +
                 ", target size: " + to_string(smallerSize) +
                 ". Unable to resize File to a larger size. " +
                 PrintFileName(m_baseName));
    return;
  }

  {
    lock_guard<recursive_mutex> lock(m_mutex);

    while (!m_pages.empty() && smallerSize < m_size) {
      auto lastPage = --m_pages.end();
      auto lastPageSize = (*lastPage)->Size();
      if (smallerSize + lastPageSize <= m_size) {
        if (!(*lastPage)->UseTempFile()) {
          m_cacheSize -= lastPageSize;
        }
        m_size -= lastPageSize;
        m_pages.erase(lastPage);
      } else {
        auto newSize = lastPageSize - (m_size - smallerSize);
        // Do a lazy remove for last page.
        (*lastPage)->ResizeToSmallerSize(newSize);
        if (!(*lastPage)->UseTempFile()) {
          m_cacheSize -= lastPageSize - newSize;
        }
        m_size -= lastPageSize - newSize;
        break;
      }
    }
  }
}

// --------------------------------------------------------------------------
void File::RemoveTempFileFromDiskIfExists(bool logOn) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  if (UseTempFile()) {
    auto tmpFile = AskTempFilePath();
    if (FileExists(tmpFile, logOn)) {
      if (logOn) {
        RemoveFileIfExists(tmpFile);
      } else {
        RemoveFileIfExistsNoLog(tmpFile);
      }
    }
  }
}

// --------------------------------------------------------------------------
void File::Clear() {
  {
    lock_guard<recursive_mutex> lock(m_mutex);
    m_pages.clear();
  }
  m_mtime.store(0);
  m_size.store(0);
  m_cacheSize.store(0);
  RemoveTempFileFromDiskIfExists(true);
  m_useTempFile.store(false);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::LowerBoundPage(off_t offset) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return LowerBoundPageNoLock(offset);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::LowerBoundPageNoLock(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset, 0, make_shared<IOStream>(0));
  return m_pages.lower_bound(tmpPage);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::UpperBoundPage(off_t offset) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  return UpperBoundPageNoLock(offset);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::UpperBoundPageNoLock(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset, 0, make_shared<IOStream>(0));
  return m_pages.upper_bound(tmpPage);
}

// --------------------------------------------------------------------------
pair<PageSetConstIterator, PageSetConstIterator> File::IntesectingRange(
    off_t off1, off_t off2) const {
  assert(off1 <= off2);
  lock_guard<recursive_mutex> lock(m_mutex);
  auto it1 = LowerBoundPageNoLock(off1);
  auto it2 = LowerBoundPageNoLock(off2);
  // Move backward it1 to pointing to the page which maybe intersect with
  // 'offset'.
  reverse_iterator<PageSetConstIterator> reverseIt(it1);
  it1 = (reverseIt == m_pages.crend() || (*reverseIt)->Next() <= off1)
            ? it1
            : (++reverseIt).base();

  return {it1, it2};
}

// --------------------------------------------------------------------------
const std::shared_ptr<Page> &File::Front() {
  lock_guard<recursive_mutex> lock(m_mutex);
  assert(!m_pages.empty());
  return *(m_pages.begin());
}

// --------------------------------------------------------------------------
const std::shared_ptr<Page> &File::Back() {
  lock_guard<recursive_mutex> lock(m_mutex);
  assert(!m_pages.empty());
  return *(m_pages.rbegin());
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, const char *buffer) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSize = 0;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(offset, len, buffer, AskTempFilePath()));
    // do not count size of data stored in tmp file
  } else {
    res = m_pages.emplace(new Page(offset, len, buffer));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;  // count size of data stored in cache
    }
  }
  if (res.second) {
    addedSize = len;
    m_size += len;
  } else {
    DebugError("Fail to new a page from a buffer " +
               ToStringLine(offset, len, buffer) + PrintFileName(m_baseName));
  }

  return make_tuple(res.first, res.second, addedSizeInCache, addedSize);
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, const shared_ptr<iostream> &stream) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSize = 0;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(offset, len, stream, AskTempFilePath()));
  } else {
    res = m_pages.emplace(new Page(offset, len, stream));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;
    }
  }
  if (res.second) {
    addedSize = len;
    m_size += len;
  } else {
    DebugError("Fail to new a page from a stream " + ToStringLine(offset, len) +
               PrintFileName(m_baseName));
  }

  return make_tuple(res.first, res.second, addedSizeInCache, addedSize);
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, shared_ptr<iostream> &&stream) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSize = 0;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(offset, len, stream, AskTempFilePath()));
  } else {
    res = m_pages.emplace(new Page(offset, len, std::move(stream)));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;
    }
  }
  if (res.second) {
    addedSize = len;
    m_size += len;
  } else {
    DebugError("Fail to new a page from a stream " + ToStringLine(offset, len) +
               PrintFileName(m_baseName));
  }

  return make_tuple(res.first, res.second, addedSizeInCache, addedSize);
}

}  // namespace Data
}  // namespace QS
