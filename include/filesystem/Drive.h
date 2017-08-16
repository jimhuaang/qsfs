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

#ifndef _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_  // NOLINT

#include <atomic>  // NOLINT
#include <memory>
#include <utility>
#include <vector>

#include "data/Directory.h"

namespace QS {

namespace Client {
class Client;
class QSClient;
class TransferManager;
}

namespace Data {
class Cache;
class DirectoryTree;
class FileMetaData;
class Node;
}

namespace FileSystem {

class Drive {
 public:
  Drive(Drive &&) = delete;
  Drive(const Drive &) = delete;
  Drive &operator=(Drive &&) = delete;
  Drive &operator=(const Drive &) = delete;
  ~Drive() = default;

 public:
  static Drive &Instance();
  // accessor
  bool IsMountable() const;
  const std::shared_ptr<QS::Client::Client> &GetClient() const {
    return m_client;
  }
  const std::unique_ptr<QS::Client::TransferManager> &GetTransferManager()
      const {
    return m_transferManager;
  }
  const std::unique_ptr<QS::Data::Cache> &GetCache() const { return m_cache; }
  const std::unique_ptr<QS::Data::DirectoryTree> &GetDirectoryTree() const {
    return m_directoryTree;
  }

 public:
  // Get the Node in directory tree by path.
  // Dir path should ending with '/'.
  // Using growDriectory to invoke growing the directory tree asynchronizely,
  // which means the children of the directory will be add to the tree.
  // Return a pair: the first member is the node, the second member denote
  // if the node is modified comparing with the moment before this operation
  std::pair<std::weak_ptr<QS::Data::Node>, bool> GetNode(
      const std::string &path, bool growDirectory = true);
  // Retrive the children of the given directory in the directory tree.
  // This will grow the directory tree synchronizely.
  std::pair<QS::Data::ChildrenMultiMapConstIterator,
            QS::Data::ChildrenMultiMapConstIterator>
  GetChildren(const std::string &dirPath);

 private:
  void GrowDirectoryTree(std::shared_ptr<QS::Data::FileMetaData> &&fileMeta);
  void GrowDirectoryTree(
      std::vector<std::shared_ptr<QS::Data::FileMetaData>> &&fileMetas);

 private:
  std::shared_ptr<QS::Client::Client> &GetClient() { return m_client; }
  std::unique_ptr<QS::Client::TransferManager> &GetTransferManager() {
    return m_transferManager;
  }
  std::unique_ptr<QS::Data::Cache> &GetCache() { return m_cache; }
  std::unique_ptr<QS::Data::DirectoryTree> &GetDirectoryTree() {
    return m_directoryTree;
  }
  // mutator
  void SetClient(std::shared_ptr<QS::Client::Client> client);
  void SetTransferManager(
      std::unique_ptr<QS::Client::TransferManager> transferManager);
  void SetCache(std::unique_ptr<QS::Data::Cache> cache);
  void SetDirectoryTree(std::unique_ptr<QS::Data::DirectoryTree> dirTree);

 private:
  Drive();
  // TODO(jim):
  // SetTransferManager();
  mutable std::atomic<bool> m_mountable;
  std::shared_ptr<QS::Client::Client> m_client;
  std::unique_ptr<QS::Client::TransferManager> m_transferManager;
  std::unique_ptr<QS::Data::Cache> m_cache;
  std::unique_ptr<QS::Data::DirectoryTree> m_directoryTree;

  friend class QS::Client::QSClient;
};

}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_
