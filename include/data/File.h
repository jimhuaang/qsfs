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

#ifndef _QSFS_FUSE_INCLUDED_DATA_FILE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_FILE_H_  // NOLINT

#include <stddef.h>  // for size_t
#include <time.h>

#include <atomic>  // NOLINT
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <tuple>
#include <utility>

#include "data/Page.h"

namespace QS {

namespace Data {
class Cache;
class Entry;

// Range represented by a pair of {offset, size}
using ContentRangeDeque = std::deque<std::pair<off_t, size_t>>;

class File {
 public:
  explicit File(const std::string &baseName, time_t mtime = 0, size_t size = 0)
      : m_baseName(baseName),
        m_mtime(mtime),
        m_size(size),
        m_cacheSize(size),
        m_useTempFile(false) {}

  File(File &&) = delete;
  File(const File &) = delete;
  File &operator=(File &&) = delete;
  File &operator=(const File &) = delete;
  ~File();

 public:
  std::string GetBaseName() const { return m_baseName; }
  size_t GetSize() const { return m_size.load(); }
  size_t GetCachedSize() const { return m_cacheSize.load(); }
  time_t GetTime() const { return m_mtime.load(); }
  bool UseTempFile() const { return m_useTempFile.load(); }

  // return tmp file path
  std::string AskTempFilePath() const;

  // Return a pair of iterators pointing to the range of consecutive pages
  // at the front of cache list
  //
  // @param  : void
  // @return : pair
  std::pair<PageSetConstIterator, PageSetConstIterator>
  ConsecutivePagesAtFront() const;

  // Whether the file containing the content
  //
  // @param  : content range start, content range size
  // @return : bool
  bool HasData(off_t start, size_t size) const;

  // Return the unexisting content ranges
  //
  // @param  : file total size
  // @return : a list of pair {range start, range size}
  ContentRangeDeque GetUnloadedRanges(uint64_t fileTotalSize) const;

 private:
  // Read from the cache (file pages)
  //
  // @param  : file offset, len of bytes, entry(meta data of file)
  // @return : a pair of {read size, page list containing data}
  //
  // If the file mtime is newer than m_mtime, clear cache and download file.
  // If any bytes is not present, download it as a new page.
  // Pagelist of outcome is sorted by page offset.
  //
  // Notes: the pagelist of outcome could containing more bytes than given
  // input asking for, for example, the 1st page of outcome could has a
  // offset which is ahead of input 'offset'.
  std::pair<size_t, std::list<std::shared_ptr<Page>>> Read(off_t offset,
                                                           size_t len,
                                                           Entry *entry);

  // Write a block of bytes into pages
  //
  // @param  : file offset, len, buffer, modification time
  // @return : {success, added size in cache, added size}
  //
  // From pointer of buffer, number of len bytes will be writen.
  // The owning file's offset is set with 'offset'.
  std::tuple<bool, size_t, size_t> Write(off_t offset, size_t len,
                                         const char *buffer, time_t mtime);

  // Write stream into pages
  //
  // @param  : file offset, len of stream, stream, modification time
  // @return : {success, added size in cache, added size}
  //
  // The stream will be moved to the pages.
  // The owning file's offset is set with 'offset'.
  std::tuple<bool, size_t, size_t> Write(
      off_t offset, size_t len, std::shared_ptr<std::iostream> &&stream,
      time_t mtime);

  // Resize the total pages' size to a smaller size.
  void ResizeToSmallerSize(size_t smallerSize);

  // Remove tmp file from disk
  void RemoveTempFileFromDiskIfExists(bool logOn = true) const;

  // Clear pages and reset attributes.
  void Clear();

  // Set modification time
  void SetTime(time_t mtime) { m_mtime.store(mtime); }

  // Set flag to use temp file
  void SetUseTempFile(bool useTempFile) { m_useTempFile.store(useTempFile); }

  // Returns an iterator pointing to the first Page that is not ahead of offset.
  // If no such Page is found, a past-the-end iterator is returned.
  // Not-Synchronized
  PageSetConstIterator LowerBoundPage(off_t offset) const;

  // Returns an iterator pointing to the first Page that is behind of offset.
  // If no such Page is found, a past-the-end iterator is returned.
  // Not-Synchronized
  PageSetConstIterator UpperBoundPage(off_t offset) const;

  // Returns a pair iterators pointing the pages which intesecting with
  // the range (from off1 to off2).
  // The first member pointing to first page not ahead of (could be
  // intersecting with) off1; the second member pointing to the first
  // page not ahead of off2, same as LowerBoundPage(off2).
  std::pair<PageSetConstIterator, PageSetConstIterator> IntesectingRange(
      off_t off1, off_t off2) const;

  // Return the first key in the page set.
  // Not-Synchronized
  const std::shared_ptr<Page> &Front() { return *(m_pages.begin()); }

  // Return the last key in the page set.
  // Not-Synchronized
  const std::shared_ptr<Page> &Back() { return *(m_pages.rbegin()); }

  // Add a new page from a block of character without checking input.
  // Return {pointer to addedpage, success, added size in cache, added size}
  // Not-Synchronized
  std::tuple<PageSetConstIterator, bool, size_t, size_t> UnguardedAddPage(
      off_t offset, size_t len, const char *buffer);
  std::tuple<PageSetConstIterator, bool, size_t, size_t> UnguardedAddPage(
      off_t offset, size_t len, const std::shared_ptr<std::iostream> &stream);
  std::tuple<PageSetConstIterator, bool, size_t, size_t> UnguardedAddPage(
      off_t offset, size_t len, std::shared_ptr<std::iostream> &&stream);

 private:
  std::string m_baseName;           // file base name
  std::atomic<time_t> m_mtime;      // time of last modification
  std::atomic<size_t> m_size;       // record sum of all pages's size
  std::atomic<size_t> m_cacheSize;  // record sum of all pages' size
                                    // stored in cache not including tmp file

  std::atomic<bool> m_useTempFile;  // use tmp file when no free cache space
  mutable std::recursive_mutex m_mutex;
  PageSet m_pages;              // a set of pages suppose to be successive

  friend class Cache;
};

}  // namespace Data
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDED_DATA_FILE_H_
