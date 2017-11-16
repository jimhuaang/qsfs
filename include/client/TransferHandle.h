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
#ifndef INCLUDE_CLIENT_TRANSFERHANDLE_H_
#define INCLUDE_CLIENT_TRANSFERHANDLE_H_

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint16_t uint64_t

#include <atomic>              // NOLINT
#include <condition_variable>  // NOLINT
#include <iostream>
#include <map>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <vector>

#include "client/ClientError.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

class TransferManager;
class TransferHandle;
class Part;
class QSTransferManager;

using PartIdToPartMap = std::map<uint16_t, std::shared_ptr<Part> >;

class Part {
 public:
  Part(uint16_t partId, size_t bestProgressInBytes, size_t sizeInBytes,
       size_t rangeBegin);
  Part() : Part(0, 0, 0, 0) {}

  Part(Part &&) = default;
  Part(const Part &) = default;
  Part &operator=(Part &&) = default;
  Part &operator=(const Part &) = default;
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

/*   std::shared_ptr<std::vector<char> > GetDownloadBuffer() const {
    return atomic_load(&m_downloadBuffer);
  } */

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

 private:
  uint16_t m_partId;
  std::string m_eTag;        // could be empty
  size_t m_currentProgress;  // in bytes
  size_t m_bestProgress;     // in bytes
  size_t m_size;             // in bytes
  size_t m_rangeBegin;

  // Notice: use atomic functions every time you touch the variable
  std::shared_ptr<std::iostream> m_downloadPartStream;

  friend class TransferHandle;
  friend class QSTransferManager;
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
  // Ctor
  TransferHandle(const std::string &bucket, const std::string &objKey,
                 size_t contentRangeBegin, uint64_t totalTransferSize,
                 TransferDirection direction,
                 const std::string &targetFilePath = std::string());

  TransferHandle() = delete;
  TransferHandle(TransferHandle &&) = delete;
  TransferHandle(const TransferHandle &) = delete;
  TransferHandle &operator=(TransferHandle &&) = delete;
  TransferHandle &operator=(const TransferHandle &) = delete;

  ~TransferHandle() { ReleaseDownloadStream(); }

 public:
  bool IsMultipart() const { return m_isMultipart; }
  const std::string &GetMultiPartId() const { return m_multipartId; }
  PartIdToPartMap GetQueuedParts() const;
  PartIdToPartMap GetPendingParts() const;
  PartIdToPartMap GetFailedParts() const;
  PartIdToPartMap GetCompletedParts() const;
  bool HasQueuedParts() const;
  bool HasPendingParts() const;
  bool HasFailedParts() const;
  bool HasParts() const;

  // TODO(jim): notes the transfer progress 's two invariants (aws)
  uint64_t GetBytesTransferred() const { return m_bytesTransferred.load(); }
  uint64_t GetBytesTotalSize() const { return m_bytesTotalSize.load(); }
  TransferDirection GetDirection() const { return m_direction; }
  bool ShouldContinue() const { return !m_cancel.load(); }
  TransferStatus GetStatus() const;

  const std::string &GetTargetFilePath() const { return m_targetFilePath; }

  const std::string &GetBucket() const { return m_bucket; }
  const std::string &GetObjectKey() const { return m_objectKey; }
  size_t GetContentRangeBegin() const { return m_contentRangeBegin; }
  const std::string &GetContentType() const { return m_contentType; }
  const std::map<std::string, std::string> &GetMetadata() const {
    return m_metadata;
  }

  const ClientError<QSError> &GetError() const { return m_error; }

 public:
  void WaitUntilFinished() const;
  bool DoneTransfer() const;

 private:
  void SetIsMultiPart(bool isMultipart) { m_isMultipart = isMultipart; }
  void SetMultipartId(const std::string &multipartId) {
    m_multipartId = multipartId;
  }
  void AddQueuePart(const std::shared_ptr<Part> &part);
  void AddPendingPart(const std::shared_ptr<Part> &part);
  void ChangePartToFailed(const std::shared_ptr<Part> &part);
  void ChangePartToCompleted(const std::shared_ptr<Part> &part,
                             const std::string &eTag = std::string());
  void UpdateBytesTransferred(uint64_t amount) { m_bytesTransferred += amount; }
  void SetBytesTotalSize(uint64_t totalSize) { m_bytesTotalSize = totalSize; }

  // Cancel transfer, this happens asynchronously, if you need to wait for it to
  // be cancelled, either handle the callbacks or call WaitUntilFinished
  void Cancle() { m_cancel.store(true); }
  // Reset the cancellation for a retry. This will be done automatically by
  // transfer manager
  void Restart() { m_cancel.store(false); }
  void UpdateStatus(TransferStatus status);

  void WritePartToDownloadStream(
      const std::shared_ptr<std::iostream> &partStream, size_t offset);

  void SetDownloadStream(std::shared_ptr<std::iostream> downloadStream) {
    atomic_store(&m_downloadStream, downloadStream);
  }
  std::shared_ptr<std::iostream> GetDownloadStream() const {
    return atomic_load(&m_downloadStream);
  }

  void ReleaseDownloadStream();
  void SetTargetFilePath(const std::string &path) { m_targetFilePath = path; }

  void SetBucket(const std::string &bucket) { m_bucket = bucket; }
  void SetObjectKey(const std::string &key) { m_objectKey = key; }
  void SetContentRangeBegin(size_t rangeBegin) {
    m_contentRangeBegin = rangeBegin;
  }
  void SetContentType(const std::string &contentType) {
    m_contentType = contentType;
  }
  void SetMetadata(const std::map<std::string, std::string> &metadata) {
    m_metadata = metadata;
  }

  void SetError(const ClientError<QSError> &error) { m_error = error; }

 private:
  bool m_isMultipart;
  std::string m_multipartId;  // mulitpart upload id
  PartIdToPartMap m_queuedParts;
  PartIdToPartMap m_pendingParts;
  PartIdToPartMap m_failedParts;
  PartIdToPartMap m_completedParts;
  mutable std::mutex m_partsLock;

  std::atomic<uint64_t> m_bytesTransferred;  // size have been transferred
  std::atomic<uint64_t> m_bytesTotalSize;    // the total size need
                                             // to be transferred
  TransferDirection m_direction;
  std::atomic<bool> m_cancel;
  TransferStatus m_status;
  mutable std::mutex m_statusLock;
  mutable std::condition_variable m_waitUntilFinishSignal;

  std::shared_ptr<std::iostream> m_downloadStream;
  mutable std::recursive_mutex m_downloadStreamLock;
  // If known, this is the location of the local file being uploaded from,
  // or downloaded to.
  // If use stream API, this will always be blank.
  std::string m_targetFilePath;

  std::string m_bucket;
  std::string m_objectKey;
  size_t m_contentRangeBegin;
  // content type of object being transferred
  std::string m_contentType;
  // In case of an upload, this is the metadata that was placed on the object.
  // In case of a download, this is the object metadata from the GET operation.
  std::map<std::string, std::string> m_metadata;

  ClientError<QSError> m_error;

  friend class QSTransferManager;
  friend class Part;
};

}  // namespace Client
}  // namespace QS


#endif  // INCLUDE_CLIENT_TRANSFERHANDLE_H_
