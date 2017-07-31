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

#include "client/TransferHandle.h"

#include <assert.h>

#include <mutex>  // NOLINT
#include <string>

#include "base/LogMacros.h"

namespace QS {

namespace Client {

using std::iostream;
using std::lock_guard;
using std::mutex;
using std::recursive_mutex;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unique_lock;

namespace {
bool IsFinishedStatus(TransferStatus status) {
  return !(status == TransferStatus::NotStarted ||
           status == TransferStatus::InProgress);
}

bool AllowTransition(TransferStatus current, TransferStatus next) {
  if (IsFinishedStatus(current) && IsFinishedStatus(next)) {
    return current == TransferStatus::Cancelled &&
           next == TransferStatus::Aborted;
  }
  return true;
}

size_t GetStreamOutputSize(const shared_ptr<iostream> &stream) {
  size_t sz = 0;
  assert(stream);
  if (stream) {
    auto curPos = stream->tellp();
    stream->seekp(0, std::ios_base::end);
    sz = static_cast<size_t>(stream->tellp() - curPos);
    stream->seekp(curPos);
  } else {
    DebugWarning("Try to lookup the size of a null output stream");
  }
  return sz;
}

size_t GetStreamInputSize(const shared_ptr<iostream> &stream) {
  size_t sz = 0;
  assert(stream);
  if (stream) {
    auto curPos = stream->tellg();
    stream->seekg(0, std::ios_base::end);
    sz = static_cast<size_t>(stream->tellg() - curPos);
    stream->seekg(curPos);
  } else {
    DebugWarning("Try to lookup the size of a null input stream");
  }
  return sz;
}

}  // namespace

Part::Part(uint16_t partId, size_t bestProgressInBytes, size_t sizeInBytes)
    : m_partId(partId),
      m_eTag(""),
      m_currentProgress(0),
      m_bestProgress(bestProgressInBytes),
      m_size(sizeInBytes),
      m_rangeBegin(0),
      m_downloadPartStream(nullptr),
      m_downloadBuffer(nullptr) {}

string Part::ToString() const {
  return "[part id: " + to_string(m_partId) + ", etag: " + m_eTag +
         ", current progress(bytes): " + to_string(m_currentProgress) +
         ", best progress(bytes): " + to_string(m_bestProgress) +
         ", size(bytes): " + to_string(m_size) +
         ", range begin: " + to_string(m_rangeBegin) + "]";
}

void Part::OnDataTransferred(uint64_t amount,
                             const shared_ptr<TransferHandle> &handle) {
  m_currentProgress += static_cast<size_t>(amount);
  if (m_currentProgress > m_bestProgress) {
    handle->UpdateBytesTransferred(m_currentProgress - m_bestProgress);
    m_bestProgress = m_currentProgress;
  }
}

const PartIdToPartMap &TransferHandle::GetQueuedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_queuedParts;
}

const PartIdToPartMap &TransferHandle::GetPendingParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_pendingParts;
}

const PartIdToPartMap &TransferHandle::GetFailedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_failedParts;
}

const PartIdToPartMap &TransferHandle::GetCompletedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_completedParts;
}

bool TransferHandle::HasQueuedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return !m_queuedParts.empty();
}

bool TransferHandle::HasPendingParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return !m_pendingParts.empty();
}

bool TransferHandle::HasFailedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return !m_failedParts.empty();
}

bool TransferHandle::HasParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return !m_failedParts.empty() || !m_queuedParts.empty() ||
         !m_pendingParts.empty();
}

TransferStatus TransferHandle::GetStatus() const {
  lock_guard<mutex> lock(m_statusLock);
  return m_status;
}

void TransferHandle::AddQueuePart(const shared_ptr<Part> &part) {
  lock_guard<mutex> lock(m_partsLock);
  part->Reset();
  m_failedParts.erase(part->GetPartId());
  auto res = m_queuedParts.emplace(part->GetPartId(), part);
  if (!res.second) {
    DebugWarning("Fail to add to queue parts with part " + part->ToString());
  }
}

void TransferHandle::AddPendingPart(const shared_ptr<Part> &part) {
  lock_guard<mutex> lock(m_partsLock);
  m_queuedParts.erase(part->GetPartId());
  auto res = m_pendingParts.emplace(part->GetPartId(), part);
  if (!res.second) {
    DebugWarning("Fail to add to pending parts with part " + part->ToString());
  }
}

void TransferHandle::ChangePartToFailed(const shared_ptr<Part> &part) {
  auto partId = part->GetPartId();
  lock_guard<mutex> lock(m_partsLock);
  part->Reset();
  m_queuedParts.erase(partId);
  m_pendingParts.erase(partId);
  auto res = m_failedParts.emplace(partId, part);
  if (!res.second) {
    DebugWarning("Fail to change part state to failed with part " +
                 part->ToString());
  }
}

void TransferHandle::ChangePartToCompleted(const shared_ptr<Part> &part,
                                           const string &eTag) {
  auto partId = part->GetPartId();
  lock_guard<mutex> lock(m_partsLock);
  if (m_pendingParts.erase(partId) == 0) {
    m_failedParts.erase(partId);
  }
  part->SetETag(eTag);
  auto res = m_completedParts.emplace(partId, part);
  if (!res.second) {
    DebugWarning("Fail to change part state to completed with part " +
                 part->ToString());
  }
}

void TransferHandle::UpdateStatus(TransferStatus newStatus) {
  unique_lock<mutex> lock(m_statusLock);
  if (AllowTransition(m_status, newStatus)) {
    m_status = newStatus;
    if (IsFinishedStatus(newStatus)) {
      if (newStatus == TransferStatus::Completed) {
        ReleaseDownloadStream();
      }
      lock.unlock();
      m_waitUntilFinishSignal.notify_all();
    }
  }
}

void TransferHandle::WaitUntilFinished() const {
  unique_lock<mutex> lock(m_statusLock);
  m_waitUntilFinishSignal.wait(lock, [this] {
    return IsFinishedStatus(m_status) && !HasPendingParts();
  });
}

size_t TransferHandle::GetDownloadStreamOutputSize() const {
  lock_guard<recursive_mutex> lock(m_downloadStreamLock);
  return GetStreamOutputSize(m_downloadStream);
}

void TransferHandle::WritePartToDownloadStream(
    const shared_ptr<iostream> &partStream, size_t offset) {
  lock_guard<recursive_mutex> lock(m_downloadStreamLock);
  assert(m_downloadStream && partStream);
  if (m_downloadStream) {
    auto downloadStreamOutputSize = GetDownloadStreamOutputSize();
    if (offset >= downloadStreamOutputSize) {
      DebugError("Overflow: write offset " + to_string(offset) +
                 " is larger than download stream output size " +
                 to_string(downloadStreamOutputSize));
      return;
    }
    if (partStream) {
      auto partStreamSize = GetStreamInputSize(partStream);
      if (offset + partStreamSize >= downloadStreamOutputSize) {
        DebugError("Overflow: wirte offset " + to_string(offset) +
                   " part stream input size " + to_string(partStreamSize) +
                   " download stream output size " +
                   to_string(downloadStreamOutputSize));
        return;
      }
      partStream->seekg(0);
      m_downloadStream->seekp(offset);
      (*m_downloadStream) << partStream->rdbuf();
      m_downloadStream->flush();
    } else {
      DebugError(
          "Try to write part to download stream with a null stream input");
      return;
    }
  } else {
    DebugError("Try to write part to a null download stream");
  }
}

void TransferHandle::SetDownloadStream(
    const shared_ptr<iostream> &downloadStream) {
  lock_guard<recursive_mutex> lock(m_downloadStreamLock);
  m_downloadStream = downloadStream;
}

void TransferHandle::ReleaseDownloadStream() {
  lock_guard<recursive_mutex> lock(m_downloadStreamLock);
  if (m_downloadStream) {
    m_downloadStream->flush();
    m_downloadStream.reset();
  }
}

}  // namespace Client
}  // namespace QS
