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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_TRANSFERMANAGER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_TRANSFERMANAGER_H_  // NOLINT

#include "data/ResourceManager.h"

#include <iostream>
#include <memory>

#include "data/Size.h"

namespace QS {

namespace Data {
class Entry;
class ResourceManager;
}  // namespace Data

namespace FileSystem {
class Drive;
}  // namespace FileSystem

namespace Threading {
class ThreadPool;
}  // namespace Threading

namespace Client {

class Client;
class TransferHandle;

static const size_t DEFAULT_MAX_PARALLEL_TRANSFERS = 5;
static const uint64_t DEFAULT_MAX_BUF_HEAP_SIZE = QS::Data::Size::MB50;
static const uint64_t DEFAULT_MAX_BUF_SIZE = QS::Data::Size::MB5;

struct TransferManagerConfigure {
  // Maximum size of the working buffers to use, default 50MB
  uint64_t m_bufferMaxHeapSize = DEFAULT_MAX_BUF_HEAP_SIZE;

  // Memory size allocated for one transfer buffer, default 5MB
  // If you are uploading large files(e.g. larger than 50GB), this needs to be
  // specified to be a size larger than 5MB. And keeping in mind that you may
  // need to increase your max heap size if you plan on increasing buffer size.
  uint64_t m_bufferSize = DEFAULT_MAX_BUF_SIZE;

  // Maximum number of file transfers to run in parallel, deafult 5.
  size_t m_maxParallelTransfers = DEFAULT_MAX_PARALLEL_TRANSFERS;

  TransferManagerConfigure(
      uint64_t bufMaxHeapSize = DEFAULT_MAX_BUF_HEAP_SIZE,
      uint64_t bufSize = DEFAULT_MAX_BUF_SIZE,
      size_t maxParallelTransfers = DEFAULT_MAX_PARALLEL_TRANSFERS)
      : m_bufferMaxHeapSize(bufMaxHeapSize),
        m_bufferSize(bufSize),
        m_maxParallelTransfers(maxParallelTransfers) {}
};

class TransferManager {
 public:
  TransferManager(const TransferManagerConfigure &config);
  TransferManager(TransferManager &&) = delete;
  TransferManager(const TransferManager &) = delete;
  TransferManager &operator=(TransferManager &&) = delete;
  TransferManager &operator=(const TransferManager &) = delete;
  virtual ~TransferManager();

 public:
  virtual std::shared_ptr<TransferHandle> UploadFile() = 0;
  virtual std::shared_ptr<TransferHandle> RetryUpload() = 0;
  virtual void UploadDirectory(const std::string &directory) = 0;

  virtual std::shared_ptr<TransferHandle> DownloadFile(
      const QS::Data::Entry &entry, off_t offset, size_t size,
      std::shared_ptr<std::iostream> downloadStream) = 0;
  virtual std::shared_ptr<TransferHandle> RetryDownload() = 0;
  virtual void DownloadDirectory(const std::string &directory) = 0;

  virtual void AbortMultipartUpload(
      const std::shared_ptr<TransferHandle> &handle) = 0;

 public:
  uint64_t GetBufferMaxHeapSize() const {
    return m_configure.m_bufferMaxHeapSize;
  }
  uint64_t GetBufferSize() const { return m_configure.m_bufferSize; }
  size_t GetMaxParallelTransfers() const {
    return m_configure.m_maxParallelTransfers;
  }
  size_t GetBufferCount() const;

 protected:
  std::shared_ptr<Client> GetClient() { return m_client; }
  std::unique_ptr<QS::Threading::ThreadPool> &GetExecutor() {
    return m_executor;
  }
  std::unique_ptr<QS::Data::ResourceManager> &GetBufferManager() {
    return m_bufferManager;
  }

 private:
  void SetClient(const std::shared_ptr<Client> &client);
  friend class QS::FileSystem::Drive;

 private:
  void InitializeResources();

 private:
  TransferManagerConfigure m_configure;
  std::unique_ptr<QS::Data::ResourceManager> m_bufferManager;

  // This executor is used in a different context with the client used one.
  std::unique_ptr<QS::Threading::ThreadPool> m_executor;
  std::shared_ptr<Client> m_client;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_TRANSFERMANAGER_H_
