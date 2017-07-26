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

#include "client/Transfer.h"

#include <mutex>  // NOLINT
#include <string>

#include "base/LogMacros.h"

namespace QS {

namespace Client {

using std::lock_guard;
using std::mutex;
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
}  // namespace

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

PartIdToPartUnorderedMap TransferHandle::GetQueuedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_queuedParts;
}

PartIdToPartUnorderedMap TransferHandle::GetPendingParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_pendingParts;
}

PartIdToPartUnorderedMap TransferHandle::GetFailedParts() const {
  lock_guard<mutex> lock(m_partsLock);
  return m_failedParts;
}

PartIdToPartUnorderedMap TransferHandle::GetCompletedParts() const {
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

void TransferHandle::AddQueuePart(const std::shared_ptr<Part> &part) {
  lock_guard<mutex> lock(m_partsLock);
  part->Reset();
  m_failedParts.erase(part->GetPartId());
  auto res = m_queuedParts.emplace(part->GetPartId(), part);
  if (!res.second) {
    DebugWarning("Fail to add to queue parts with part " + part->ToString());
  }
}

void TransferHandle::AddPendingPart(const std::shared_ptr<Part> &part) {
  lock_guard<mutex> lock(m_partsLock);
  m_queuedParts.erase(part->GetPartId());
  auto res = m_pendingParts.emplace(part->GetPartId(), part);
  if (!res.second) {
    DebugWarning("Fail to add to pending parts with part " + part->ToString());
  }
}

void TransferHandle::ChangePartToFailed(const std::shared_ptr<Part> &part) {
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

void TransferHandle::ChangePartToCompleted(const std::shared_ptr<Part> &part,
                                           const std::string &eTag) {
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
      if(newStatus == TransferStatus::Completed){
        CleanupDownloadStream();
      }
      lock.unlock();
      m_waitUntilFinishSignal.notify_all();
    }
  }
}

void TransferHandle::WaitUntilFinished() const {
  unique_lock<mutex> lock(m_statusLock);
  m_waitUntilFinishSignal.wait(
      lock, [this] { return IsFinishedStatus(m_status) && !HasPendingParts(); });
}

void TransferHandle::CleanupDownloadStream() {
  // TODO(jim):
}

}  // namespace Client
}  // namespace QS
