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

#include <assert.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "base/LogMacros.h"
#include "client/ClientConfiguration.h"
#include "client/TransferHandle.h"
#include "data/Directory.h"
#include "data/IOStream.h"

namespace QS {

namespace Client {

using std::iostream;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::to_string;

shared_ptr<TransferHandle> QSTransferManager::UploadFile() { return nullptr; }
shared_ptr<TransferHandle> QSTransferManager::RetryUpload() { return nullptr; }
void QSTransferManager::UploadDirectory(const std::string &directory) {}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::DownloadFile(
    const QS::Data::Entry &entry, off_t offset, size_t size,
    shared_ptr<iostream> downloadStream) {
  // Check size
  string bucket = ClientConfiguration::Instance().GetBucket();
  string filePath = entry.GetFilePath();
  auto fileSize = entry.GetFileSize();
  size_t downloadSize = size;
  size_t remainingSize = 0;
  if (offset + size > fileSize) {
    DebugWarning("Input overflow [file:offset:size:totalsize = " + filePath +
                 ":" + to_string(offset) + ":" + to_string(size) + ":" +
                 to_string(fileSize) + "]. Ajust it");
    downloadSize = fileSize - offset;
  } else {
    remainingSize = fileSize - (offset + size);
  }

  // Do download synchronizely for requested file part
  auto handle = PrepareDownload(bucket, filePath, downloadSize);
  handle->SetDownloadStream(downloadStream);
  DoDownload(handle);

  // following the aws case to only set downloadstream for single case
  // otherwise use the buffer, at the end combine them to download stream

  return handle;
}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::RetryDownload() {
  return nullptr;
}

// --------------------------------------------------------------------------
void QSTransferManager::DownloadDirectory(const std::string &directory) {}

void QSTransferManager::AbortMultipartUpload(
    const shared_ptr<TransferHandle> &handle) {}

// --------------------------------------------------------------------------
shared_ptr<TransferHandle> QSTransferManager::PrepareDownload(
    const string &bucket, const string &objKey, size_t downloadSize) {
  auto handle = std::make_shared<TransferHandle>(bucket, objKey);
  // prepare part and add it into queue
  assert(GetBufferSize() > 0);
  size_t partCount = std::ceil(downloadSize / GetBufferSize());
  handle->SetIsMultiPart(partCount > 1);
  for (size_t i = 1; i < partCount; ++i) {
    // part id, best progress in bytes, part size
    auto part = make_shared<Part>(i, 0, GetBufferSize());
    part->SetRangeBegin((i - 1) * GetBufferSize());
    handle->AddQueuePart(part);
  }
  size_t sz = downloadSize - (partCount - 1) * GetBufferSize();
  handle->AddQueuePart(
      make_shared<Part>(partCount, 0, std::min(sz, GetBufferSize())));
  return handle;
}

// --------------------------------------------------------------------------
void QSTransferManager::DoSinglePartDownload(
    const shared_ptr<TransferHandle> &handle) {
  //
}

// --------------------------------------------------------------------------
void QSTransferManager::DoMultiPartDownload(
    const shared_ptr<TransferHandle> &handle) {
  //
}

// --------------------------------------------------------------------------
void QSTransferManager::DoDownload(const shared_ptr<TransferHandle> &handle) {
  if (handle->IsMultipart()) {
    DoMultiPartDownload(handle);
  } else {
    DoSinglePartDownload(handle);
  }
}

}  // namespace Client
}  // namespace QS
