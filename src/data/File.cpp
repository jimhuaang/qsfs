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
#include <stdio.h>  // for pclose

#include <iterator>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "client/TransferHandle.h"
#include "client/TransferManager.h"
#include "data/Directory.h"
#include "data/IOStream.h"
#include "filesystem/Configure.h"
#include "filesystem/Drive.h"

namespace QS {

namespace Data {

using QS::Data::Entry;
using QS::FileSystem::Configure::GetCacheTemporaryDirectory;
using QS::StringUtils::PointerAddress;
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
// @param  : file base name, file offset, file page size
// @return : string
//
// Notes: tmp file path format as following,
//        e.g: /tmp/basename1024_4096
//             1024 is offset, 4096 is size in bytes
string BuildTempFilePath(const string &basename, off_t offset, size_t size){
  string qsfsTmpDir = GetCacheTemporaryDirectory();
  return qsfsTmpDir + basename + to_string(offset) + "_" + to_string(size);
}

// --------------------------------------------------------------------------
string PrintFileName(const string& file){
  return "[file=" + file + "]";
}

}  // namespace


// --------------------------------------------------------------------------
pair<PageSetConstIterator, PageSetConstIterator> File::ConsecutivePagesAtFront()
    const {
  lock_guard<recursive_mutex> lock(m_mutex);
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
ContentRangeDeque File::GetUnloadedRanges(uint64_t fileTotalSize) const {
  lock_guard<recursive_mutex> lock(m_mutex);
  ContentRangeDeque ranges;
  auto cur = m_pages.begin();
  auto next = m_pages.begin();
  while (++next != m_pages.end()) {
    if ((*cur)->Next() < (*next)->Offset()) {
      off_t off = (*cur)->Next();
      size_t size = static_cast<size_t>((*next)->Offset() - off);
      ranges.emplace_back(off, size);
    }
    ++cur;
  }

  if (static_cast<size_t>((*cur)->Next()) < fileTotalSize) {
    off_t off = (*cur)->Next();
    size_t size = static_cast<size_t>(fileTotalSize - off);
    ranges.emplace_back(off, size);
  }

  return ranges;
}

// --------------------------------------------------------------------------
pair<size_t, list<shared_ptr<Page>>> File::Read(off_t offset, size_t len,
                                                Entry *entry) {
  // Cache already check input.
  // bool isValidInput = offset >= 0 && len > 0 && entry != nullptr && (*entry);
  // assert(isValidInput);
  // if (!isValidInput) {
  //   DebugError("Fail to read file with invalid input " +
  //              ToStringLine(offset, len));
  //   return {0, list<shared_ptr<Page>>()};
  // }

  time_t mtime = entry->GetMTime();
  string filePath = entry->GetFilePath();
  size_t outcomeSize = 0;
  list<shared_ptr<Page>> outcomePages;
  auto LoadFileAndAddPage = [this, mtime, filePath, &outcomeSize,
                             &outcomePages](off_t offset, size_t len) {
    auto stream = make_shared<IOStream>(len);
    auto handle =
        QS::FileSystem::Drive::Instance().GetTransferManager()->DownloadFile(
            filePath, offset, len, stream);

    if (handle) {
      handle->WaitUntilFinished();
      if (handle->DoneTransfer() && !handle->HasFailedParts()) {
        auto res = UnguardedAddPage(offset, len, stream);
        if (std::get<1>(res)) {
          SetTime(mtime);
          outcomePages.emplace_back(*(std::get<0>(res)));
          outcomeSize += (*(std::get<0>(res)))->m_size;
        }
      }
    }
  };

  if (mtime > 0) {
    // File is just created, update mtime.
    if (m_mtime == 0) {
      SetTime(mtime);
    } else if (mtime > m_mtime) {
      // Detected modification in the file, clear file.
      Clear();
      entry->SetFileSize(0);
    }
  }

  // Adjust the length.
  if (entry->GetFileSize() > 0) {
    auto len_ = static_cast<size_t>(entry->GetFileSize() - offset);
    if (len_ < len) {
      len = len_;
      DebugWarning("Try to read file([fileId:size] = [" + entry->GetFilePath() +
                   ":" + to_string(entry->GetFileSize()) + "]) with input " +
                   ToStringLine(offset, len) + " which surpass the file size");
    }
  }

  {
    lock_guard<recursive_mutex> lock(m_mutex);

    // If pages is empty.
    if (m_pages.empty()) {
      LoadFileAndAddPage(offset, len);
      return {outcomeSize, outcomePages};
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
      if (offset_ < page->m_offset) {  // Load new page for bytes not present.
        auto lenNewPage = page->m_offset - offset_;
        LoadFileAndAddPage(offset_, lenNewPage);
        offset_ = page->m_offset;
        len_ -= lenNewPage;
      } else {  // Collect existing pages.
        if (len_ <= static_cast<size_t>(page->Next() - offset_)) {
          outcomePages.emplace_back(page);
          outcomeSize += page->m_size;
          return {outcomeSize, outcomePages};
        } else {
          outcomePages.emplace_back(page);
          outcomeSize += page->m_size;

          offset_ = page->Next();
          len_ -= page->m_size;
          ++it1;
        }
      }
    }  // end of while
    // Load new page for bytes not present.
    if (len_ > 0) {
      LoadFileAndAddPage(offset_, len_);
    }
  }  // end of lock_guard

  return {outcomeSize, outcomePages};
}

// --------------------------------------------------------------------------
pair<bool, size_t> File::Write(off_t offset, size_t len, const char *buffer,
                               time_t mtime) {
  // Cache has checked input.
  // bool isValidInput = = offset >= 0 && len > 0 &&  buffer != NULL;
  // assert(isValidInput);
  // if (!isValidInput) {
  //   DebugError("Fail to write file with invalid input " +
  //              ToStringLine(offset, len, buffer));
  //   return false;
  // }

  size_t addedSizeInCache = 0;
  auto AddPageAndUpdateTime = [this, mtime, &addedSizeInCache](
      off_t offset, size_t len, const char *buffer) -> bool {
    auto res = this->UnguardedAddPage(offset, len, buffer);
    if (std::get<1>(res)) {
      this->SetTime(mtime);
      addedSizeInCache += std::get<2>(res);
    }
    return std::get<1>(res);
  };

  lock_guard<recursive_mutex> lock(m_mutex);
  // If pages is empty.
  if (m_pages.empty()) {
    return {AddPageAndUpdateTime(offset, len, buffer), addedSizeInCache};
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
        return {false, addedSizeInCache};
      } else {
        addedSizeInCache += std::get<2>(res);
      }

      offset_ = page->m_offset;
      start_ += lenNewPage;
      len_ -= lenNewPage;
    } else {  // Refresh the overlapped page's content.
      if (len_ <= static_cast<size_t>(page->Next() - offset_)) {
        SetTime(mtime);
        // refresh parital content of page
        return {page->UnguardedRefresh(offset_, len_, buffer + start_),
                addedSizeInCache};
      } else {
        // refresh entire page
        auto refresh = page->UnguardedRefresh(buffer + start_);
        if (!refresh) {
          success = false;
          return {false, addedSizeInCache};
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
      addedSizeInCache += std::get<2>(res);
    } else {
      success = false;
    }
  }
  SetTime(mtime);

  return {success, addedSizeInCache};
}

// --------------------------------------------------------------------------
pair<bool, size_t> File::Write(off_t offset, size_t len,
                               shared_ptr<iostream> &&stream, time_t mtime) {
  auto AddPageAndUpdateTime = [this, mtime](
      off_t offset, size_t len,
      shared_ptr<iostream> &&stream) -> pair<bool, size_t> {
    auto res = this->UnguardedAddPage(offset, len, std::move(stream));
    if (std::get<1>(res)) {
      this->SetTime(mtime);
    }
    return {std::get<1>(res), std::get<2>(res)};
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
      // replace old stream
      page->SetStream(std::move(stream));
      SetTime(mtime);
      return {true, 0};
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
  if(smallerSize == curSize){
    return;
  }
  if (smallerSize > curSize) {
    DebugWarning(
        "File size: " + to_string(curSize) +
        ", target size: " + to_string(smallerSize) +
        ". Unable to resize File to a larger size. " + PrintFileName(m_baseName));
    return;
  }

  off_t offset = static_cast<off_t>(smallerSize - 1);
  {
    lock_guard<recursive_mutex> lock(m_mutex);

    // Erase pages behind offset
    auto it = UpperBoundPage(offset);
    if (it != m_pages.end()) {
      for (auto page = it; page != m_pages.end(); ++page) {
        m_cacheSize -= (*page)->m_size;
      }
      m_pages.erase(it, m_pages.end());
    }

    if (!m_pages.empty()) {
      auto &lastPage = Back();
      if (offset == lastPage->Stop()) {
        // Do nothing.
        return;
      } else if (lastPage->m_offset <= offset && offset < lastPage->Stop()) {
        auto newSize = smallerSize - lastPage->m_offset;
        m_cacheSize -= lastPage->m_size - newSize;
        // Do a lazy remove for last page.
        lastPage->ResizeToSmallerSize(newSize);
      } else {
        DebugError("After erased pages behind offset " + to_string(offset) +
                   " , last page now with " +
                   ToStringLine(lastPage->m_offset, lastPage->m_size) +
                   ". Should not happen, but go on. " +
                   PrintFileName(m_baseName));
      }
    } else {
      DebugError("File becomes empty after erase pages behind offset of " +
                 to_string(offset) + ". Should not happen, but go on. " +
                 PrintFileName(m_baseName));
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
  m_cacheSize.store(0);
  m_useTempFile.store(false);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::LowerBoundPage(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset, 0, make_shared<IOStream>(0));
  return m_pages.lower_bound(tmpPage);
}

// --------------------------------------------------------------------------
PageSetConstIterator File::UpperBoundPage(off_t offset) const {
  auto tmpPage = make_shared<Page>(offset, 0, make_shared<IOStream>(0));
  return m_pages.upper_bound(tmpPage);
}

// --------------------------------------------------------------------------
pair<PageSetConstIterator, PageSetConstIterator> File::IntesectingRange(
    off_t off1, off_t off2) const {
  auto it1 = LowerBoundPage(off1);
  auto it2 = LowerBoundPage(off2);
  // Move backward it1 to pointing to the page which maybe intersect with
  // 'offset'.
  reverse_iterator<PageSetConstIterator> reverseIt(it1);
  it1 = (reverseIt == m_pages.crend() || (*reverseIt)->Next() <= off1)
            ? it1
            : (++reverseIt).base();

  return {it1, it2};
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, const char *buffer) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(
        offset, len, buffer, BuildTempFilePath(GetBaseName(), offset, len)));
    // do not count size of data stored in tmp file
  } else {
    res = m_pages.emplace(new Page(offset, len, buffer));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;  // count size of data stored in cache
    }
  }

  DebugErrorIf(!res.second,
               "Fail to new a page from a buffer " +
                   ToStringLine(offset, len, buffer) +
                   PrintFileName(m_baseName));
  return make_tuple(res.first, res.second, addedSizeInCache);
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, const shared_ptr<iostream> &stream) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(
        offset, len, stream, BuildTempFilePath(GetBaseName(), offset, len)));
  } else {
    res = m_pages.emplace(new Page(offset, len, stream));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;
    }
  }
  DebugErrorIf(!res.second,
               "Fail to new a page from a stream " + ToStringLine(offset, len) +
                   PrintFileName(m_baseName));
  return make_tuple(res.first, res.second, addedSizeInCache);
}

// --------------------------------------------------------------------------
tuple<PageSetConstIterator, bool, size_t> File::UnguardedAddPage(
    off_t offset, size_t len, shared_ptr<iostream> &&stream) {
  pair<PageSetConstIterator, bool> res;
  size_t addedSizeInCache = 0;
  if (UseTempFile()) {
    res = m_pages.emplace(new Page(
        offset, len, stream, BuildTempFilePath(GetBaseName(), offset, len)));
  } else {
    res = m_pages.emplace(new Page(offset, len, std::move(stream)));
    if (res.second) {
      addedSizeInCache = len;
      m_cacheSize += len;
    }
  }
  DebugErrorIf(!res.second,
               "Fail to new a page from a stream " + ToStringLine(offset, len) +
                   PrintFileName(m_baseName));
  return make_tuple(res.first, res.second, addedSizeInCache);
}

}  // namespace Data
}  // namespace QS
