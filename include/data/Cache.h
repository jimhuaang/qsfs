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

#include <stddef.h>  // for size_t
#include <time.h>

#include <sys/types.h>  // for off_t

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/HashUtils.h"
#include "data/File.h"
#include "data/Page.h"

namespace QS {

namespace Client {
class QSClient;
}  // namespace Client

namespace FileSystem {
class Drive;
}  // namespace FileSystem

namespace Data {

class Node;

using FileIdToFilePair = std::pair<std::string, std::unique_ptr<File>>;
using CacheList = std::list<FileIdToFilePair>;
using CacheListIterator = CacheList::iterator;
using CacheListConstIterator = CacheList::const_iterator;
using FileIdToCacheListIteratorMap =
    std::unordered_map<std::string, CacheListIterator, HashUtils::StringHash>;

class Cache {
 public:
  Cache() = default;
  Cache(Cache &&) = default;
  Cache(const Cache &) = delete;
  Cache &operator=(Cache &&) = default;
  Cache &operator=(const Cache &) = delete;
  ~Cache() = default;

 public:
  // Has available free space
  //
  // @param  : need size
  // @return : bool
  //
  // If cache files' size plus needSize surpass MaxCacheSize,
  // then there is no avaiable needSize space.
  bool HasFreeSpace(size_t needSize) const;  // size in byte

  // Is the last file in cache open
  //
  // @param  : void
  // @return : bool
  //
  // NOTES:
  // we put least recently used File at back and free the cache starting 
  // from back, so IsLastFileOpen can be used as a condition when freeing cache.
  bool IsLastFileOpen() const;

  // Whether the file content existing
  //
  // @param  : file path, content range start, content range size
  // @return : bool
  bool HasFileData(const std::string &filePath, off_t start, size_t size) const;

  // Whether a file exists in cache
  //
  // @param  : file path
  // @return : bool
  bool HasFile(const std::string &filePath) const;

  // Return the unexisting content ranges for a given file
  //
  // @param  : file path, file total size
  // @return : a list of {range start, range size}
  ContentRangeDeque GetUnloadedRanges(const std::string &filePath,
                                      uint64_t fileTotalSize) const;

  // Return the number of files in cache
  int GetNumFile() const;

  // Get cache size
  size_t GetSize() const { return m_size; }

  // Get file mtime
  time_t GetTime(const std::string &fileId) const;

  // Find the file
  //
  // @param  : file path (absolute path)
  // @return : const iterator point to cache list
  CacheListIterator Find(const std::string &filePath);

  // Begin of cache list
  CacheListIterator Begin();

  // End of cache list
  CacheListIterator End();

  // Read file cache into a buffer
  //
  // @param  : file path, offset, len, buffer, node
  // @return : size of bytes have been writen to buffer
  //
  // If not found fileId in cache, create it in cache and load its pages.
  size_t Read(const std::string &fileId, off_t offset, size_t len, char *buffer,
              std::shared_ptr<Node> node);
 private:

  // Write a block of bytes into file cache
  //
  // @param  : file path, file offset, len, buffer, modification time
  // @return : bool
  //
  // If File of fileId doesn't exist, create one.
  // From pointer of buffer, number of len bytes will be writen.
  bool Write(const std::string &fileId, off_t offset, size_t len,
             const char *buffer, time_t mtime);

  // Write stream into file cache
  //
  // @param  : file path, file offset, stream, modification time
  // @return : bool
  //
  // If File of fileId doesn't exist, create one.
  // Stream will be moved to cache.
  bool Write(const std::string &fileId, off_t offset, size_t len,
             std::shared_ptr<std::iostream> &&stream, time_t mtime);

  // Free cache space
  //
  // @param  : size need to be freed
  // @return : bool
  //
  // Discard the least recently used File to make sure
  // there will be number of size avaiable cache space.
  bool Free(size_t size);  // size in byte

  // Remove file from cache
  //
  // @param  : file id
  // @return : iterator pointing to next file in cache list if remove sucessfully,
  //           otherwise return past-the-end iterator.
  CacheListIterator Erase(const std::string &fildId);

  // Rename a file
  //
  // @param  : file id, new file id
  // @return : void
  void Rename(const std::string &oldFileId, const std::string &newFileId);

  // Change file mtime
  //
  // @param  : file id, mtime
  // @return : void
  void SetTime(const std::string &fileId, time_t mtime);

  // Resize a file
  //
  // @param  : file id, new file size
  // @return : void
  void Resize(const std::string &fileId, size_t newSize);

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

  friend class QS::Client::QSClient;
  friend class QS::FileSystem::Drive;
};

}  // namespace Data
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDE_DATA_CACHE_H_
