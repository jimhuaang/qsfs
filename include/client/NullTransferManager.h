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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_NULLTRANSFERMANAGER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_NULLTRANSFERMANAGER_H_  // NOLINT

#include "client/TransferManager.h"

#include <memory>

namespace QS {

namespace Data {
class Entry;
class ResourceManager;
}  // namespace Data

namespace Threading {
class ThreadPool;
}  // namespace Threading

namespace Client {

class TransferHandle;

class NullTransferManager : public TransferManager {
 public:
  NullTransferManager(const TransferManagerConfigure &config)
      : TransferManager(config) {}
  NullTransferManager(NullTransferManager &&) = delete;
  NullTransferManager(const NullTransferManager &) = delete;
  NullTransferManager &operator=(NullTransferManager &&) = delete;
  NullTransferManager &operator=(const NullTransferManager &) = delete;
  ~NullTransferManager() = default;

 public:
  std::shared_ptr<TransferHandle> DownloadFile(
      const std::string &filePath, off_t offset, uint64_t size,
      std::shared_ptr<std::iostream> downStream, bool async = false) override {
    return nullptr;
  }

  std::shared_ptr<TransferHandle> RetryDownload(
      const std::shared_ptr<TransferHandle> &handle,
      std::shared_ptr<std::iostream> bufStream, bool async = false) override {
    return nullptr;
  }

  std::shared_ptr<TransferHandle> UploadFile(const std::string &filePath,
                                             uint64_t fileSize,
                                             bool async = false) override {
    return nullptr;
  }

  std::shared_ptr<TransferHandle> RetryUpload(
      const std::shared_ptr<TransferHandle> &handle,
      bool async = false) override {
    return nullptr;
  }

  void AbortMultipartUpload(
      const std::shared_ptr<TransferHandle> &handle) override {}
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_NULLTRANSFERMANAGER_H_
