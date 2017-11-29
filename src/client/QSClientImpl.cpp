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

#include "client/QSClientImpl.h"

#include <assert.h>

#include <chrono>  // NOLINT
#include <future>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "qingstor/Bucket.h"
#include "qingstor/HttpCommon.h"
#include "qingstor/QingStor.h"
#include "qingstor/QsConfig.h"
#include "qingstor/QsErrors.h"  // for sdk QsError
#include "qingstor/Types.h"     // for sdk QsOutput

#include "base/LogMacros.h"
#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"
#include "client/QSClient.h"
#include "client/QSError.h"
#include "client/Utils.h"

namespace QS {

namespace Client {

using QingStor::AbortMultipartUploadInput;
using QingStor::AbortMultipartUploadOutput;
using QingStor::Bucket;
using QingStor::CompleteMultipartUploadInput;
using QingStor::CompleteMultipartUploadOutput;
using QingStor::DeleteMultipleObjectsInput;
using QingStor::DeleteMultipleObjectsOutput;
using QingStor::DeleteObjectInput;
using QingStor::DeleteObjectOutput;
using QingStor::GetBucketStatisticsInput;
using QingStor::GetBucketStatisticsOutput;
using QingStor::GetObjectInput;
using QingStor::GetObjectOutput;
using QingStor::HeadBucketInput;
using QingStor::HeadBucketOutput;
using QingStor::HeadObjectInput;
using QingStor::HeadObjectOutput;
using QingStor::Http::HttpResponseCode;
using QingStor::InitiateMultipartUploadInput;
using QingStor::InitiateMultipartUploadOutput;
using QingStor::ListMultipartInput;
using QingStor::ListMultipartOutput;
using QingStor::ListMultipartUploadsInput;
using QingStor::ListMultipartUploadsOutput;
using QingStor::ListObjectsInput;
using QingStor::ListObjectsOutput;
using QingStor::PutObjectInput;
using QingStor::PutObjectOutput;
using QingStor::QsOutput;
using QingStor::UploadMultipartInput;
using QingStor::UploadMultipartOutput;
using QS::Client::Utils::ParseRequestContentRange;
using QS::Client::Utils::ParseResponseContentRange;
using std::chrono::milliseconds;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace {

// --------------------------------------------------------------------------
ClientError<QSError> BuildQSError(QsError sdkErr, const string &exceptionName,
                                  const QsOutput &output, bool retriable) {
  // QS_ERR_NO_ERROR only says the request has sent, but it desen't not mean
  // response code is ok
  auto rspCode = const_cast<QsOutput &>(output).GetResponseCode();
  auto err = SDKResponseToQSError(rspCode);
  if (err == QSError::UNKNOWN) {
    err = SDKErrorToQSError(sdkErr);
  }

  return ClientError<QSError>(err, exceptionName,
                              // sdk does not provide const-qualified accessor
                              SDKResponseCodeToString(rspCode), retriable);
}

// --------------------------------------------------------------------------
ClientError<QSError> TimeOutError(const string &exceptionName,
                                  std::future_status status) {
  ClientError<QSError> err;
  switch (status) {
    case std::future_status::timeout:
      // request timeout is retryable
      err =
          ClientError<QSError>(QSError::REQUEST_EXPIRED, exceptionName,
                               QSErrorToString(QSError::REQUEST_EXPIRED), true);
      break;
    case std::future_status::deferred:
      err = ClientError<QSError>(QSError::REQUEST_DEFERRED, exceptionName,
                                 QSErrorToString(QSError::REQUEST_DEFERRED),
                                 false);
      break;
    case std::future_status::ready:  // Bypass
    default:
      err = ClientError<QSError>(QSError::GOOD, exceptionName,
                                 QSErrorToString(QSError::GOOD), false);
      break;
  }

  return err;
}

}  // namespace

// --------------------------------------------------------------------------
QSClientImpl::QSClientImpl() : ClientImpl() {
  if (!m_bucket) {
    const auto &clientConfig = ClientConfiguration::Instance();
    const auto &qsConfig = QSClient::GetQingStorConfig();
    m_bucket = unique_ptr<Bucket>(new Bucket(*qsConfig,
                                             clientConfig.GetBucket(),
                                             clientConfig.GetZone()));
  }
}

// --------------------------------------------------------------------------
GetBucketStatisticsOutcome QSClientImpl::GetBucketStatistics(
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorGetBucketStatistics";
  auto DoGetBucketStatistics =
      [this]() -> pair<QsError, GetBucketStatisticsOutput> {
    GetBucketStatisticsInput input;  // dummy input
    GetBucketStatisticsOutput output;
    auto sdkErr = m_bucket->GetBucketStatistics(input, output);
    return {sdkErr, std::move(output)};
  };
  auto fGetBucketStatistics =
      GetExecutor()->SubmitCallablePrioritized(DoGetBucketStatistics);
  auto fStatus = fGetBucketStatistics.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fGetBucketStatistics.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return GetBucketStatisticsOutcome(std::move(output));
    } else {
      return GetBucketStatisticsOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return GetBucketStatisticsOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
HeadBucketOutcome QSClientImpl::HeadBucket(uint32_t msTimeDuration,
                                           bool useThreadPool) const {
  string exceptionName = "QingStorHeadBucket";
  auto DoHeadBucket = [this]() -> pair<QsError, HeadBucketOutput> {
    HeadBucketInput input;  // dummy input
    HeadBucketOutput output;
    auto sdkErr = m_bucket->HeadBucket(input, output);
    return {sdkErr, std::move(output)};
  };
  std::future<pair<QsError, HeadBucketOutput>> fHeadBucket;
  if (useThreadPool) {
    fHeadBucket = GetExecutor()->SubmitCallablePrioritized(DoHeadBucket);
  } else {
    fHeadBucket = std::async(std::launch::async, DoHeadBucket);
  }
  auto fStatus = fHeadBucket.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fHeadBucket.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return HeadBucketOutcome(std::move(output));
    } else {
      return HeadBucketOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return HeadBucketOutcome(std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
ListObjectsOutcome QSClientImpl::ListObjects(ListObjectsInput *input,
                                             bool *resultTruncated,
                                             uint64_t *resCount,
                                             uint64_t maxCount,
                                             uint32_t msTimeDuration,
                                             bool useThreadPool) const {
  string exceptionName = "QingStorListObjects";
  if (input == nullptr) {
    return ListObjectsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null ListObjectsInput", false));
  }
  exceptionName.append(" prefix=");
  exceptionName.append(input->GetPrefix());

  if (input->GetLimit() <= 0) {
    return ListObjectsOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_OBJECTS, exceptionName,
        "ListObjectsInput with negative count limit", false));
  }

  if (resultTruncated != nullptr) {
    *resultTruncated = false;
  }
  if (resCount != nullptr) {
    *resCount = 0;
  }

  bool listAllObjects = maxCount == 0;
  uint64_t count = 0;
  bool responseTruncated = true;
  vector<ListObjectsOutput> result;
  do {
    if (!listAllObjects) {
      int remainingCount = static_cast<int>(maxCount - count);
      if (remainingCount < input->GetLimit()) {
        input->SetLimit(remainingCount);
      }
    }

    auto DoListObjects = [this, input]() -> pair<QsError, ListObjectsOutput> {
      ListObjectsOutput output;
      auto sdkErr = m_bucket->ListObjects(*input, output);
      return {sdkErr, std::move(output)};
    };
    std::future<pair<QsError, ListObjectsOutput>> fListObjects;
    if (useThreadPool) {
      fListObjects = GetExecutor()->SubmitCallablePrioritized(DoListObjects);
    } else {
      fListObjects = std::async(std::launch::async, DoListObjects);
    }
    auto fStatus = fListObjects.wait_for(milliseconds(msTimeDuration));
    if (fStatus == std::future_status::ready) {
      auto res = fListObjects.get();
      auto &sdkErr = res.first;
      auto &output = res.second;

      auto responseCode = output.GetResponseCode();
      if (SDKResponseSuccess(sdkErr, responseCode)) {
        count += output.GetKeys().size();
        count += output.GetCommonPrefixes().size();
        responseTruncated = !output.GetNextMarker().empty();
        if (responseTruncated) {
          input->SetMarker(output.GetNextMarker());
        }
        result.push_back(std::move(output));
      } else {
        return ListObjectsOutcome(std::move(BuildQSError(
            sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
      }
    } else {
      return ListObjectsOutcome(
          std::move(TimeOutError(exceptionName, fStatus)));
    }
  } while (responseTruncated && (listAllObjects || count < maxCount));
  if (resultTruncated != nullptr) {
    *resultTruncated = responseTruncated;
  }
  if (resCount != nullptr) {
    *resCount = count;
  }
  return ListObjectsOutcome(std::move(result));
}

// --------------------------------------------------------------------------
DeleteMultipleObjectsOutcome QSClientImpl::DeleteMultipleObjects(
    DeleteMultipleObjectsInput *input, uint32_t msTimeDuration) const {
  static const char *exceptionName = "QingStorDeleteMultipleObjects";
  if (input == nullptr) {
    return DeleteMultipleObjectsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null DeleteMultiplueObjectsInput", false));
  }

  auto DoDeleteMultiObject =
      [this, input]() -> pair<QsError, DeleteMultipleObjectsOutput> {
    DeleteMultipleObjectsOutput output;
    auto sdkErr = m_bucket->DeleteMultipleObjects(*input, output);
    return {sdkErr, std::move(output)};
  };
  auto fDeleteMultiObject =
      GetExecutor()->SubmitCallablePrioritized(DoDeleteMultiObject);
  auto fStatus = fDeleteMultiObject.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fDeleteMultiObject.get();
    auto &sdkErr = res.first;
    auto &output = res.second;

    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      // TODO(jim): check undeleted objects
      return DeleteMultipleObjectsOutcome(std::move(output));
    } else {
      return DeleteMultipleObjectsOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return DeleteMultipleObjectsOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
ListMultipartUploadsOutcome QSClientImpl::ListMultipartUploads(
    QingStor::ListMultipartUploadsInput *input, bool *resultTruncated,
    uint64_t maxCount, uint32_t msTimeDuration) const {
  string exceptionName = "QingStorListMultipartUploads";
  if (input == nullptr) {
    return ListMultipartUploadsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null ListMultipartUploadsInput", false));
  }

  exceptionName.append(" prefix=");
  exceptionName.append(input->GetPrefix());

  if (input->GetLimit() <= 0) {
    return ListMultipartUploadsOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_MULTIPART_UPLOADS, exceptionName,
        "ListMultipartUploadsInput with negative count limit", false));
  }
  if (resultTruncated != nullptr) {
    *resultTruncated = false;
  }

  bool listAllPartUploads = maxCount == 0;
  uint64_t count = 0;
  bool responseTruncated = true;
  vector<ListMultipartUploadsOutput> result;
  do {
    if (!listAllPartUploads) {
      int remainingCount = static_cast<int>(maxCount - count);
      if (remainingCount < input->GetLimit()) input->SetLimit(remainingCount);
    }

    auto DoListMultipartUploads =
        [this, input]() -> pair<QsError, ListMultipartUploadsOutput> {
      ListMultipartUploadsOutput output;
      auto sdkErr = m_bucket->ListMultipartUploads(*input, output);
      return {sdkErr, std::move(output)};
    };
    auto fListMultiPartUploads =
        GetExecutor()->SubmitCallablePrioritized(DoListMultipartUploads);
    auto fStatus = fListMultiPartUploads.wait_for(milliseconds(msTimeDuration));
    if (fStatus == std::future_status::ready) {
      auto res = fListMultiPartUploads.get();
      auto &sdkErr = res.first;
      auto &output = res.second;
      auto responseCode = output.GetResponseCode();
      if (SDKResponseSuccess(sdkErr, responseCode)) {
        count += output.GetUploads().size();
        responseTruncated = !output.GetNextKeyMarker().empty();
        if (responseTruncated) {
          input->SetKeyMarker(output.GetNextKeyMarker());
        }
        result.push_back(std::move(output));
      } else {
        return ListMultipartUploadsOutcome(std::move(BuildQSError(
            sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
      }
    } else {
      return ListMultipartUploadsOutcome(
          std::move(TimeOutError(exceptionName, fStatus)));
    }
  } while (responseTruncated && (listAllPartUploads || count < maxCount));
  if (resultTruncated != nullptr) {
    *resultTruncated = responseTruncated;
  }
  return ListMultipartUploadsOutcome(std::move(result));
}

// --------------------------------------------------------------------------
DeleteObjectOutcome QSClientImpl::DeleteObject(const string &objKey,
                                               uint32_t msTimeDuration) const {
  string exceptionName = "QingStorDeleteObject";
  if (objKey.empty()) {
    return DeleteObjectOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName, "Empty ObjectKey", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoDeleteObject = [this, objKey]() -> pair<QsError, DeleteObjectOutput> {
    DeleteObjectInput input;  // dummy input
    DeleteObjectOutput output;
    auto sdkErr = m_bucket->DeleteObject(objKey, input, output);
    return {sdkErr, std::move(output)};
  };

  auto fDeleteObject = GetExecutor()->SubmitCallablePrioritized(DoDeleteObject);
  auto fStatus = fDeleteObject.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fDeleteObject.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return DeleteObjectOutcome(std::move(output));
    } else {
      return DeleteObjectOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return DeleteObjectOutcome(std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
GetObjectOutcome QSClientImpl::GetObject(const std::string &objKey,
                                         GetObjectInput *input,
                                         uint32_t msTimeDuration) const {
  string exceptionName = "QingStorGetObject";
  if (objKey.empty() || input == nullptr) {
    return GetObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null GetObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  bool askPartialContent = !input->GetRange().empty();

  auto DoGetObject = [this, objKey, input]() -> pair<QsError, GetObjectOutput> {
    GetObjectOutput output;
    auto sdkErr = m_bucket->GetObject(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };

  auto fGetObject = GetExecutor()->SubmitCallablePrioritized(DoGetObject);
  auto fStatus = fGetObject.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fGetObject.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      if (askPartialContent) {
        // qs sdk specification: if request set with range parameter, then
        // response successful with code 206 (Partial Content)
        if (output.GetResponseCode() != HttpResponseCode::PARTIAL_CONTENT) {
          return GetObjectOutcome(
              std::move(BuildQSError(sdkErr, exceptionName, output, true)));
        } else {
          size_t reqLen = ParseRequestContentRange(input->GetRange()).second;
          // auto rspRes = ParseResponseContentRange(output.GetContentRange());
          size_t rspLen = output.GetContentLength();
          DebugWarningIf(
              rspLen < reqLen,
              "[content range request:response=" + input->GetRange() + ":" +
                  output.GetContentRange() + "]");
        }
      }
      return GetObjectOutcome(std::move(output));
    } else {
      return GetObjectOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return GetObjectOutcome(std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
HeadObjectOutcome QSClientImpl::HeadObject(const string &objKey,
                                           HeadObjectInput *input,
                                           uint32_t msTimeDuration) const {
  string exceptionName = "QingStorHeadObject";
  if (objKey.empty() || input == nullptr) {
    return HeadObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null HeadObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoHeadObject = [this, objKey,
                       input]() -> pair<QsError, HeadObjectOutput> {
    HeadObjectOutput output;
    auto sdkErr = m_bucket->HeadObject(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };

  auto fHeadObject = GetExecutor()->SubmitCallablePrioritized(DoHeadObject);
  auto fStatus = fHeadObject.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fHeadObject.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return HeadObjectOutcome(std::move(output));
    } else {
      return HeadObjectOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }

  } else {
    return HeadObjectOutcome(std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
PutObjectOutcome QSClientImpl::PutObject(const string &objKey,
                                         PutObjectInput *input,
                                         uint32_t msTimeDuration) const {
  string exceptionName = "QingStorPutObject";
  if (objKey.empty() || input == nullptr) {
    return PutObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null PutObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoPutObject = [this, objKey, input]() -> pair<QsError, PutObjectOutput> {
    PutObjectOutput output;
    auto sdkErr = m_bucket->PutObject(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };

  auto fPutObject = GetExecutor()->SubmitCallablePrioritized(DoPutObject);
  auto fStatus = fPutObject.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fPutObject.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return PutObjectOutcome(std::move(output));
    } else {
      return PutObjectOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return PutObjectOutcome(std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
InitiateMultipartUploadOutcome QSClientImpl::InitiateMultipartUpload(
    const string &objKey, QingStor::InitiateMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorInitiateMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return InitiateMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null InitiateMultipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoInitiateMultipartUpload =
      [this, objKey, input]() -> pair<QsError, InitiateMultipartUploadOutput> {
    InitiateMultipartUploadOutput output;
    auto sdkErr = m_bucket->InitiateMultipartUpload(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };
  auto fInitMultipartUpload =
      GetExecutor()->SubmitCallablePrioritized(DoInitiateMultipartUpload);
  auto fStatus = fInitMultipartUpload.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fInitMultipartUpload.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return InitiateMultipartUploadOutcome(std::move(output));
    } else {
      return InitiateMultipartUploadOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return InitiateMultipartUploadOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
UploadMultipartOutcome QSClientImpl::UploadMultipart(
    const string &objKey, UploadMultipartInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorUploadMultipart";
  if (objKey.empty() || input == nullptr) {
    return UploadMultipartOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null UploadMultipartInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoUploadMultipart = [this, objKey,
                            input]() -> pair<QsError, UploadMultipartOutput> {
    UploadMultipartOutput output;
    auto sdkErr = m_bucket->UploadMultipart(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };
  auto fUploadMultipart =
      GetExecutor()->SubmitCallablePrioritized(DoUploadMultipart);
  auto fStatus = fUploadMultipart.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fUploadMultipart.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return UploadMultipartOutcome(std::move(output));
    } else {
      return UploadMultipartOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return UploadMultipartOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
CompleteMultipartUploadOutcome QSClientImpl::CompleteMultipartUpload(
    const string &objKey, CompleteMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorCompleteMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return CompleteMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null CompleteMutlipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoCompleteMultipartUpload =
      [this, objKey, input]() -> pair<QsError, CompleteMultipartUploadOutput> {
    CompleteMultipartUploadOutput output;
    auto sdkErr = m_bucket->CompleteMultipartUpload(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };
  auto fCompleteMultipartUpload =
      GetExecutor()->SubmitCallablePrioritized(DoCompleteMultipartUpload);
  auto fStatus =
      fCompleteMultipartUpload.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fCompleteMultipartUpload.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return CompleteMultipartUploadOutcome(std::move(output));
    } else {
      return CompleteMultipartUploadOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return CompleteMultipartUploadOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
AbortMultipartUploadOutcome QSClientImpl::AbortMultipartUpload(
    const string &objKey, AbortMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorAbortMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return AbortMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null AbortMultipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  auto DoAbortMultipartUpload =
      [this, objKey, input]() -> pair<QsError, AbortMultipartUploadOutput> {
    AbortMultipartUploadOutput output;
    auto sdkErr = m_bucket->AbortMultipartUpload(objKey, *input, output);
    return {sdkErr, std::move(output)};
  };
  auto fAbortMultipartUpload =
      GetExecutor()->SubmitCallablePrioritized(DoAbortMultipartUpload);
  auto fStatus = fAbortMultipartUpload.wait_for(milliseconds(msTimeDuration));
  if (fStatus == std::future_status::ready) {
    auto res = fAbortMultipartUpload.get();
    auto &sdkErr = res.first;
    auto &output = res.second;
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return AbortMultipartUploadOutcome(std::move(output));
    } else {
      return AbortMultipartUploadOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } else {
    return AbortMultipartUploadOutcome(
        std::move(TimeOutError(exceptionName, fStatus)));
  }
}

// --------------------------------------------------------------------------
ListMultipartOutcome QSClientImpl::ListMultipart(
    const string &objKey, QingStor::ListMultipartInput *input,
    bool *resultTruncated, uint64_t maxCount, uint32_t msTimeDuration) const {
  string exceptionName = "QingStorListMultipart";
  if (objKey.empty() || input == nullptr) {
    return ListMultipartOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null ListMultipartInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  if (input->GetLimit() <= 0) {
    return ListMultipartOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_MULTIPART, exceptionName,
        "ListMultipartInput with negative count limit", false));
  }
  if (resultTruncated != nullptr) {
    *resultTruncated = false;
  }

  bool listAllParts = maxCount == 0;
  uint64_t count = 0;
  bool responseTruncated = true;
  vector<ListMultipartOutput> result;
  do {
    if (!listAllParts) {
      int remainingCount = static_cast<int>(maxCount - count);
      if (remainingCount < input->GetLimit()) input->SetLimit(remainingCount);
    }

    auto DoListMultipart = [this, objKey,
                            input]() -> pair<QsError, ListMultipartOutput> {
      ListMultipartOutput output;
      auto sdkErr = m_bucket->ListMultipart(objKey, *input, output);
      return {sdkErr, std::move(output)};
    };
    auto fListMultipart =
        GetExecutor()->SubmitCallablePrioritized(DoListMultipart);
    auto fStatus = fListMultipart.wait_for(milliseconds(msTimeDuration));
    if (fStatus == std::future_status::ready) {
      auto res = fListMultipart.get();
      auto &sdkErr = res.first;
      auto &output = res.second;
      auto responseCode = output.GetResponseCode();
      if (SDKResponseSuccess(sdkErr, responseCode)) {
        count += output.GetCount();
        auto objParts = output.GetObjectParts();
        responseTruncated = !objParts.empty();
        if (!objParts.empty()) {
          input->SetPartNumberMarker(objParts.back().GetPartNumber());
        }
        result.push_back(std::move(output));
      } else {
        return ListMultipartOutcome(std::move(BuildQSError(
            sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
      }
    } else {
      return ListMultipartOutcome(
          std::move(TimeOutError(exceptionName, fStatus)));
    }
  } while (responseTruncated && (listAllParts || count < maxCount));
  if (resultTruncated != nullptr) {
    *resultTruncated = responseTruncated;
  }
  return ListMultipartOutcome(std::move(result));
}

// --------------------------------------------------------------------------
void QSClientImpl::SetBucket(unique_ptr<QingStor::Bucket> bucket) {
  assert(bucket);
  FatalIf(!bucket, "Setting a NULL bucket!");
  m_bucket = std::move(bucket);
}

}  // namespace Client
}  // namespace QS
