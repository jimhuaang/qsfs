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
#ifndef _QSFS_FUSE_INCLUDED_CLIENT_TRANSFER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_TRANSFER_H_  // NOLINT

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint16_t uint64_t

#include <atomic>              // NOLINT
#include <condition_variable>  // NOLINT
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <vector>

#include "data/ResourceManager.h"

namespace QS {

namespace Threading {
class ThreadPool;
}  // namespace Threading

namespace Client {

class Client;
class TransferHandle;
class Part;

// TODO(jim) : check if we need std::map here
using PartIdToPartUnorderedMap =
    std::unordered_map<uint16_t, std::shared_ptr<Part> >;

class Part {
 public:
  Part(uint16_t partId, size_t bestProgressInBytes, size_t sizeInBytes)
      : m_partId(partId),
        m_eTag(""),
        m_currentProgress(0),
        m_bestProgress(bestProgressInBytes),
        m_size(sizeInBytes),
        m_rangeBegin(0),
        m_downloadPartStream(nullptr),
        m_downloadBuffer(nullptr) {}

  Part() : Part(0, 0, 0) {}

  Part(Part &&) = delete;
  Part(const Part &) = delete;
  Part &operator=(Part &&) = delete;
  Part &operator=(const Part &) = delete;
  ~Part() = default;

 public:
  std::string ToString() const;
  // accessor
  uint16_t GetPartId() const { return m_partId; }
  const std::string &GetETag() const { return m_eTag; }
  size_t GetBestProgress() const { return m_bestProgress; }
  size_t GetSize() const { return m_size; }
  size_t GetRangeBegin() const { return m_rangeBegin; }

  std::shared_ptr<std::iostream> GetDownloadPartStream() const {
    return atomic_load(&m_downloadPartStream);
  }
  std::shared_ptr<std::vector<char> > GetDownloadBuffer() const {
    return atomic_load(&m_downloadBuffer);
  }

 private:
  void Reset() { m_currentProgress = 0; }
  void OnDataTransferred(uint64_t amount,
                         const std::shared_ptr<TransferHandle> &handle);
  // mutator
  void SetETag(const std::string &etag) { m_eTag = etag; }
  void SetBestProgress(size_t bestProgressInBytes) {
    m_bestProgress = bestProgressInBytes;
  }
  void SetSize(size_t sizeInBytes) { m_size = sizeInBytes; }
  void SetRangeBegin(size_t rangeBegin) { m_rangeBegin = rangeBegin; }

  void SetDownloadPartStream(
      std::shared_ptr<std::iostream> downloadPartStream) {
    atomic_store(&m_downloadPartStream, downloadPartStream);
  }
  void SetDownloadBuffer(std::shared_ptr<std::vector<char> > downloadBuffer) {
    atomic_store(&m_downloadBuffer, downloadBuffer);
  }

 private:
  uint16_t m_partId;
  std::string m_eTag;
  size_t m_currentProgress;  // in bytes
  size_t m_bestProgress;     // in bytes
  size_t m_size;             // in bytes
  size_t m_rangeBegin;

  // Notice: use atomic functions every time you touch the variable
  std::shared_ptr<std::iostream> m_downloadPartStream;
  // TODO(jim): condsider remove this
  std::shared_ptr<std::vector<char> > m_downloadBuffer;
  // std::atomic<std::iostream *> m_downloadPartStream;
  // std::atomic<std::vector<unsigned char> *> m_downloadBuffer;

  friend class TransferHandle;
};

enum class TransferStatus {
  NotStarted,  // operation is still queued and has not been processing
  InProgress,  // operation is now running
  Cancelled,   // operation is cancelled, can still be retried
  Failed,      // operation failed, can still be retried
  Completed,   // operation was sucessful
  Aborted      // operation either failed or cancelled and a user deleted the
               // multi-part upload
};

enum class TransferDirection { Upload, Download };

class TransferHandle {
 public:
  // TODO(jim): add custome ctors
  TransferHandle() = delete;
  TransferHandle(TransferHandle &&) = default;
  TransferHandle(const TransferHandle &) = default;
  TransferHandle &operator=(TransferHandle &&) = default;
  TransferHandle &operator=(const TransferHandle &) = default;

  ~TransferHandle() { CleanupDownloadStream(); };

 public:
  bool IsMultipart() const { return m_isMultipart; }
  const std::string &GetMultiPartId() const { return m_multipartId; }
  PartIdToPartUnorderedMap GetQueuedParts() const;
  PartIdToPartUnorderedMap GetPendingParts() const;
  PartIdToPartUnorderedMap GetFailedParts() const;
  PartIdToPartUnorderedMap GetCompletedParts() const;
  bool HasQueuedParts() const;
  bool HasPendingParts() const;
  bool HasFailedParts() const;
  bool HasParts() const;

  // TODO(jim): notes the transfer progress 's two invariants (aws)
  uint64_t GetBytesTransferred() const { return m_bytesTransferred.load(); }
  uint64_t GetBytesTotalSize() const { return m_bytesTotalSize; }
  TransferDirection GetDirection() const { return m_direction; }

  bool ShouldContinue() const { return !m_cancel.load(); }
  TransferStatus GetStatus() const;

 private:
  void SetIsMultiPart(bool isMultipart) { m_isMultipart = isMultipart; }
  void SetMultipartId(const std::string &multipartId) {
    m_multipartId = multipartId;
  }
  void AddQueuePart(const std::shared_ptr<Part> &part);
  void AddPendingPart(const std::shared_ptr<Part> &part);
  void ChangePartToFailed(const std::shared_ptr<Part> &part);
  void ChangePartToCompleted(const std::shared_ptr<Part> &part,
                             const std::string &eTag);
  void UpdateBytesTransferred(uint64_t amount) { m_bytesTransferred += amount; }
  void SetBytesTotalSize(uint64_t totalSize) { m_bytesTotalSize = totalSize; }

  // Cancel transfer, this happens asynchronously, if you need to wait for it to
  // be cancelled, either handle the callbacks or call WaitUntilFinished
  void Cancle() { m_cancel.store(true); }
  // Reset the cancellation for a retry. This will be done automatically by
  // TransferManager. // TODO(jim): rename transfer manager
  void Restart() { m_cancel.store(false); }
  void UpdateStatus(TransferStatus status);
  void WaitUntilFinished() const;

 private:
  void CleanupDownloadStream();

 private:
  bool m_isMultipart;
  std::string m_multipartId;
  PartIdToPartUnorderedMap m_queuedParts;
  PartIdToPartUnorderedMap m_pendingParts;
  PartIdToPartUnorderedMap m_failedParts;
  PartIdToPartUnorderedMap m_completedParts;
  mutable std::mutex m_partsLock;

  std::atomic<uint64_t> m_bytesTransferred;
  uint64_t m_bytesTotalSize;
  TransferDirection m_direction;
  std::atomic<bool> m_cancel;
  TransferStatus m_status;
  mutable std::mutex m_statusLock;
  mutable std::condition_variable m_waitUntilFinishSignal;

  std::iostream *m_downloadStream;
  std::mutex m_downloadStreamLock;

  friend class Part;

  // TODO(jim) : add other object meta info
};

class TransferManager {
 public:
  TransferManager() = default;
  TransferManager(TransferManager &&) = default;
  TransferManager(const TransferManager &) = default;
  TransferManager &operator=(TransferManager &&) = default;
  TransferManager &operator=(const TransferManager &) = default;
  ~TransferManager() = default;

 public:
  virtual std::shared_ptr<TransferHandle> UploadFile() = 0;
  virtual std::shared_ptr<TransferHandle> RetryUpload() = 0;
  virtual void UploadDirectory(const std::string &directory) = 0;

  virtual std::shared_ptr<TransferHandle> DownloadFile() = 0;
  virtual std::shared_ptr<TransferHandle> RetryDownload() = 0;
  virtual void DownloadDirectory(const std::string &directory) = 0;

  virtual void AbortMultipartUpload(
      const std::shared_ptr<TransferHandle> &handle) = 0;

 private:
  std::shared_ptr<Client> m_client;
  std::shared_ptr<QS::Threading::ThreadPool> m_executor;
  // Maximum size of the working buffers to use, default 50MB
  uint64_t m_bufferMaxHeapSize;
  // Memory size allocated for one transfer buffer, default 5MB
  // If you are uploading large files(e.g. larger than 50GB), this needs to be
  // specified to be a size larger than 5MB. And keeping in mind that you may
  // need to increase your max heap size if you plan on increasing buffer size.
  uint64_t m_bufferSize;
  // Maximum number of file transfers to run in parallel, deafult 1.
  size_t m_maxParallelTransfers;

  QS::Data::ResourceManager m_bufferManager;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_TRANSFER_H_
