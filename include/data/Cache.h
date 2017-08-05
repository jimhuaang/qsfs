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

#ifndef _QSFS_FUSE_INCLUDE_DATA_CACHE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_DATA_CACHE_H_  // NOLINT

#include <assert.h>
#include <stddef.h>
#include <time.h>

#include <sys/types.h>

#include <atomic>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/Utils.h"

namespace QS {

namespace Data {

class Entry;
class Node;

class File;

using FileIdToFilePair = std::pair<std::string, std::unique_ptr<File>>;
using CacheList = std::list<FileIdToFilePair>;
using CacheListIterator = CacheList::iterator;
using CacheListConstIterator = CacheList::const_iterator;
using FileIdToCacheListIteratorMap =
    std::unordered_map<std::string, CacheListIterator, Utils::StringHash>;

class File {
 public:
  struct Page {
   private:
    // Page attributes
    //
    // +-----------------------------------------+
    // | A File composed of two successive pages |
    // +-----------------------------------------+
    //
    // offset  stop  next   <= 1st page
    //   ^        ^  ^
    //   |________|__|________
    //   |<- size  ->|        |
    //   |___________|________|
    //   0  1  2  3  4  5  6  7
    //               ^     ^  ^
    //          offset  stop  next   <= 2nd page
    //
    // 1st Page: offset = 0, size = 4, stop = 3, next = 4
    // 2nd Page: offset = 4, size = 3, stop = 6, next = 7

    off_t m_offset = 0;  // offset from the begin of owning File
    size_t m_size = 0;   // size of bytes this page contains
    std::shared_ptr<std::iostream> m_body;  // stream storing the bytes

    // Construct Page from a block of bytes.
    // From pointer of buffer, number of len bytes will be writen.
    // The owning file's offset is 'offset'.
    Page(off_t offset, const char *buffer, size_t len,
         std::shared_ptr<std::iostream> body =
             std::make_shared<std::stringstream>());

    // Construct Page from a stream.
    // From stream, number of len bytes will be writen.
    // The owning file's offset is 'offset'.
    Page(off_t offset, const std::shared_ptr<std::iostream> &stream, size_t len,
         std::shared_ptr<std::iostream> body =
             std::make_shared<std::stringstream>());

   public:
    // Default Constructor and Constructors from stream
    explicit Page(off_t offset = 0, size_t size = 0,
                  std::shared_ptr<std::iostream> body =
                      std::make_shared<std::stringstream>())
        : m_offset(offset), m_size(size), m_body(body) {}

    Page(Page &&) = default;
    Page(const Page &) = default;
    Page &operator=(Page &&) = default;
    Page &operator=(const Page &) = default;
    ~Page() = default;

    // Return the stop position.
    off_t Stop() const { return 0 < m_size ? m_offset + m_size - 1 : 0; }

    // Return the offset of the next successive page.
    off_t Next() const { return m_offset + m_size; }

    // Refresh the page's partial content starting from 'offset - m_offset'
    // with size of 'len'.
    // May enlarge the page's size depended on 'len'.
    bool Refresh(off_t offset, char *buffer, size_t len);

    // Read the page's partial content starting from 'offset - m_offset'
    // with size of 'len', with checking.
    size_t Read(off_t offset, char *buffer, size_t len);

   private:
    // Refreseh the page's partial content starting from 'offset - m_offset'
    // with size of 'len', without checking.
    bool UnguardedRefresh(off_t offset, char *buffer, size_t len);

    // Refresh the page's partial content staring from 'offset - m_offset'
    // with page's remaining size, without checking.
    bool UnguardedRefresh(off_t offset, char *buffer) {
      return UnguardedRefresh(offset, buffer, Next() - offset);
    }

    // Refresh the page's entire content with bytes from buffer,
    // without checking.
    bool UnguardedRefresh(char *buffer) {
      return UnguardedRefresh(m_offset, buffer, m_size);
    }

    // Do a lazy resize for page.
    void Resize(size_t smallerSize);

    // Read the page's partial content starting from 'offset - m_offset'
    // with size of 'len', without checking.
    size_t UnguardedRead(off_t offset, char *buffer, size_t len);

    // Read the page's partial content starting from 'offset - m_offset'
    // with page's remaining size, without checking.
    size_t UnguardedRead(off_t offset, char *buffer) {
      return UnguardedRead(offset, buffer, Next() - offset);
    }

    // Read the page's partial content starting from 'm_offset'
    // with size of 'len', without checking.
    size_t UnguardedRead(char *buffer, size_t len) {
      return UnguardedRead(m_offset, buffer, len);
    }

    // Read the page's entire content to buffer.
    size_t UnguardedRead(char *buffer) {
      return UnguardedRead(m_offset, buffer, m_size);
    }

    friend class File;
    friend class Cache;
  };

  struct PageCmp {
    bool operator()(const std::shared_ptr<Page> &a,
                    const std::shared_ptr<Page> &b) const {
      return a->m_offset < b->m_offset;
    }
  };

  using PageSet = std::set<std::shared_ptr<Page>, PageCmp>;
  using PageSetConstIterator = PageSet::const_iterator;
  // ReadOutcome pair with first member denotes the size of bytes and the
  // second member pointing to the pages containing the bytes.
  using ReadOutcome = std::pair<size_t, std::list<std::shared_ptr<Page>>>;

  // TODO(jim): move to transfer
  // LoadOutcome paie with first member denotes the size of bytes and the
  // second member pointing to the response stream.
  // This maybe need to change to handle multipule upload
  using LoadOutcome = std::pair<size_t, std::shared_ptr<std::iostream>>;

 public:
  explicit File(time_t mtime = 0, size_t size = 0)
      : m_mtime(mtime), m_size(size) {}

  File(File &&) = delete;
  File(const File &) = delete;
  File &operator=(File &&) = delete;
  File &operator=(const File &) = delete;
  ~File() = default;

 public:
  size_t GetSize() const { return m_size.load(); }

 private:
  // Read from the cache (file pages), given a file offset and len of bytes.
  // If any bytes is not present, load it as a new page.
  // Pagelist of outcome is sorted by page offset.
  // Notes the pagelist of outcome could containing more bytes than given
  // input asking for, for example, the 1st page of outcome could has a
  // offset which is ahead of input 'offset'.
  ReadOutcome Read(off_t offset, size_t len, std::unique_ptr<Entry> *entry);

  // Load from local file cache or send request to sdk
  // TODO(jim) : move to transfer, and fileId probably should be replaced
  // to what has stored on qingstor
  // This probobaly should be moved to transfer
  LoadOutcome LoadFile(const std::string &fileId, off_t offset, size_t len);

  // Write a block of bytes into pages.
  // From pointer of buffer, number of len bytes will be writen.
  // The owning file's offset is 'offset'.
  bool Write(off_t offset, char *buffer, size_t len, time_t mtime);

  // Resize the total pages' size to a smaller size.
  void Resize(size_t smallerSize);

  // Clear pages and reset attributes.
  void Clear();

  void SetTime(time_t mtime) { m_mtime.store(mtime); }

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
  // Not-Synchronized
  std::pair<PageSetConstIterator, bool> UnguardedAddPage(off_t offset,
                                                         char *buffer,
                                                         size_t len);
  std::pair<PageSetConstIterator, bool> UnguardedAddPage(
      off_t offset, const std::shared_ptr<std::iostream> &stream, size_t len);

 private:
  std::atomic<time_t> m_mtime;  // time of last modification
  std::atomic<size_t> m_size;   // record sum of all pages' size
  std::recursive_mutex m_mutex;

  PageSet m_pages;  // a set of pages suppose to be successive

  friend class Cache;
};

class Cache {
 public:
  Cache() = default;
  Cache(Cache &&) = default;
  Cache(const Cache &) = delete;
  Cache &operator=(Cache &&) = default;
  Cache &operator=(const Cache &) = delete;
  ~Cache() = default;

 private:
  // Read file cache into a buffer.
  // If not found fileId in cache, create it in cache and load its pages.
  // Number of len bytes will be writen to buffer.
  size_t Read(const std::string &fileId, off_t offset, char *buffer, size_t len,
              std::shared_ptr<Node> node);

  // Write a block of bytes into cache File with offset of 'offset'.
  // If File of fileId doesn't exist, create one.
  // From pointer of buffer, number of len bytes will be writen.
  bool Write(const std::string &fileId, off_t offset, char *buffer, size_t len,
             time_t mtime);

  // Discard the least recently used File to make sure
  // there will be number of size avaiable cache space.
  bool Free(size_t size);  // size in byte

  CacheListIterator Erase(const std::string &fildId);
  void Rename(const std::string &oldFileId, const std::string &newFileId);
  void SetTime(const std::string &fileId, time_t mtime);
  void Resize(const std::string &fileId, size_t newSize);

 public:
  // If cache files' size plus needSize surpass MaxCacheSize,
  // then there is no avaiable needSize space.
  bool HasFreeSpace(size_t needSize) const;  // size in byte

  size_t GetSize() const { return m_size; }

 private:
  // Create an empty File with fileId in cache, without checking input.
  // If success return reference to insert file, else return m_cache.end().
  CacheListIterator UnguardedNewEmptyFile(const std::string &fileId);

  // Erase the file denoted by pos, without checking input.
  CacheListIterator UnguardedErase(FileIdToCacheListIteratorMap::iterator pos);

  // Move the file denoted by pos into the front of the cache,
  // without checking input.
  CacheListIterator UnguardedMakeFileMostRecentlyUsed(
      CacheListConstIterator pos);

 private:
  // Record sum of the cache files' size
  size_t m_size = 0;

  // Most recently used File is put at front,
  // Least recently used File is put at back.
  CacheList m_cache;

  FileIdToCacheListIteratorMap m_map;
};

}  // namespace Data
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDE_DATA_CACHE_H_
