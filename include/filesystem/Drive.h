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

namespace QS {

namespace Client{
  class Client;
  class TransferManager;
}

namespace Data{
  class Cache;
  class DirectoryTree;
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
  bool IsMountable() const;

 private:
  Drive();
  // TODO(jim):
  // SetTransferManager();
  std::atomic<bool> m_mountable;
  std::shared_ptr<QS::Client::Client> m_client;
  std::shared_ptr<QS::Client::TransferManager> m_transferManager;
  std::shared_ptr<QS::Data::Cache> m_cache;
  std::shared_ptr<QS::Data::DirectoryTree> m_directoryTree;
};

}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_DRIVE_H_