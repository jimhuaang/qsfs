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

#include "client/QSTransferManager.h"

#include "client/TransferHandle.h"

namespace QS {

namespace Client {

using std::shared_ptr;

shared_ptr<TransferHandle> QSTransferManager::UploadFile() { return nullptr; }
shared_ptr<TransferHandle> QSTransferManager::RetryUpload() { return nullptr; }
void QSTransferManager::UploadDirectory(const std::string &directory) {}

shared_ptr<TransferHandle> QSTransferManager::DownloadFile() { return nullptr; }
shared_ptr<TransferHandle> QSTransferManager::RetryDownload() {
  return nullptr;
}
void QSTransferManager::DownloadDirectory(const std::string &directory) {}

void QSTransferManager::AbortMultipartUpload(
    const shared_ptr<TransferHandle> &handle) {}

}  // namespace Client
}  // namespace QS
