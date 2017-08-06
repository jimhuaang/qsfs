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

#ifndef _QSFS_FUSE_INCLUDED_DATA_FILEMETADATAMANAGER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_FILEMETADATAMANAGER_H_  // NOLINT

#include "data/FileMetaData.h"

#include <assert.h>

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <base/Utils.h>

namespace QS {

namespace Data {

class DirectoryTree;

using FileIdToMetaDataPair =
    std::pair<std::string, std::shared_ptr<FileMetaData>>;
using MetaDataList = std::list<FileIdToMetaDataPair>;
using MetaDataListIterator = MetaDataList::iterator;
using MetaDataListConstIterator = MetaDataList::const_iterator;
using FileIdToMetaDataListIteratorMap =
    std::unordered_map<std::string, MetaDataListIterator, Utils::StringHash>;

class FileMetaDataManager {
 public:
  FileMetaDataManager(FileMetaDataManager &&) = delete;
  FileMetaDataManager(const FileMetaDataManager &) = delete;
  FileMetaDataManager &operator=(FileMetaDataManager &&) = delete;
  FileMetaDataManager &operator=(const FileMetaDataManager &) = delete;
  ~FileMetaDataManager() = default;

 public:
  static FileMetaDataManager &Instance();

 public:
  // Get file meta data
  MetaDataListConstIterator Get(const std::string &fileName) const;
  // Return begin of meta data list
  MetaDataListConstIterator Begin() const;
  // Return end of meta data list
  MetaDataListConstIterator End() const;
  // Has file meta data
  bool Has(const std::string &fileName) const;
  // If the meta data list size plus needEntryCount surpass
  // MaxFileMetaDataEntrys then there is no available space
  bool HasFreeSpace(size_t needEntryCount) const;

 private:
  // Get file meta data
  MetaDataListIterator Get(const std::string &fileName);
  // Return begin of meta data list
  MetaDataListIterator Begin();
  // Return end of meta data list
  MetaDataListIterator End();
  // Add file meta data
  // Return the iterator to the new added entry (should be the begin)
  // if sucessfullly, otherwise return the end iterator.
  MetaDataListIterator Add(std::shared_ptr<FileMetaData> &&fileMetaData);
  // Add file meta data array
  // Return the iterator to the new added entry (should be the begin) 
  // if sucessfully, otherwise return the end iterator
  // Notes: to obey 'the most recently used meta is always put at front',
  // the sequence of the input array will be reversed.
  MetaDataListIterator Add(
      std::vector<std::shared_ptr<FileMetaData>> &&fileMetaDatas);
  // Remove file meta data
  MetaDataListIterator Erase(const std::string &fileName);
  // Remvoe all file meta datas
  void Clear();

 private:
  // internal use only
  MetaDataListIterator UnguardedMakeMetaDataMostRecentlyUsed(
      MetaDataListConstIterator pos);
  bool HasFreeSpaceNoLock(size_t needEntryCount) const;
  bool FreeNoLock(size_t needEntryCount);
  MetaDataListIterator AddNoLock(std::shared_ptr<FileMetaData> &&fileMetaData);

 private:
  FileMetaDataManager();

  // Most recently used meta data is put at front,
  // Least recently used meta data in put at back.
  MetaDataList m_metaDatas;
  FileIdToMetaDataListIteratorMap m_map;
  size_t m_maxEntrys;  // max count of meta data entrys

  mutable std::recursive_mutex m_mutex;

  friend class QS::Data::DirectoryTree;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_DATA_FILEMETADATAMANAGER_H_
