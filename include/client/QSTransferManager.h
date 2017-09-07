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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSTRANSFERMANAGER_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSTRANSFERMANAGER_H_  // NOLINT

#include "client/TransferManager.h"

#include <memory>

namespace QS {

namespace Client {

class TransferHandle;

class QSTransferManager : public TransferManager {
 public:
  QSTransferManager(const TransferManagerConfigure &config)
      : TransferManager(config) {}
  QSTransferManager(QSTransferManager &&) = delete;
  QSTransferManager(const QSTransferManager &) = delete;
  QSTransferManager &operator=(QSTransferManager &&) = delete;
  QSTransferManager &operator=(const QSTransferManager &) = delete;
  ~QSTransferManager() = default;

 public:
  // Download a file
  //
  // @param  : file path, file offset, size, bufStream
  // @return : transfer handle
  std::shared_ptr<TransferHandle> DownloadFile(
      const std::string &filePath, off_t offset, uint64_t size,
      std::shared_ptr<std::iostream> bufStream) override;

  // Retry a failed download
  //
  // @param  : transfer handle to retry
  // @return : transfer handle after been retried
  std::shared_ptr<TransferHandle> RetryDownload(
      const std::shared_ptr<TransferHandle> &handle,
      std::shared_ptr<std::iostream> bufStream) override;

  // Upload a file
  //
  // @param  : file path, file size
  // @return : transfer handle
  std::shared_ptr<TransferHandle> UploadFile(const std::string &filePath,
                                             uint64_t fileSize) override;

  // Retry a failed upload
  //
  // @param  : tranfser handle to retry
  // @return : transfer handle after been retried
  std::shared_ptr<TransferHandle> RetryUpload(
      const std::shared_ptr<TransferHandle> &handle) override;

  // Abort a multipart upload
  //
  // @param  : tranfer handle to abort
  // @return : void
  //
  // By default, multipart upload will remain in a Failed state if they fail,
  // or a Cancelled state if they were cancelled. Leaving failed state around
  // still costs the owner of the bucket money. If you know you will not going
  // to retry it, abort the multipart upload request after cancelled or failed.
  void AbortMultipartUpload(
      const std::shared_ptr<TransferHandle> &handle) override;

 private:
  bool PrepareDownload(const std::shared_ptr<TransferHandle> &handle);
  void DoSinglePartDownload(const std::shared_ptr<TransferHandle> &handle);
  void DoMultiPartDownload(const std::shared_ptr<TransferHandle> &handle);
  void DoDownload(const std::shared_ptr<TransferHandle> &handle);

  bool PrepareUpload(const std::shared_ptr<TransferHandle> &handle);
  void DoSinglePartUpload(const std::shared_ptr<TransferHandle> &handle);
  void DoMultiPartUpload(const std::shared_ptr<TransferHandle> &handle);
  void DoUpload(const std::shared_ptr<TransferHandle> &handle);
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSTRANSFERMANAGER_H_
