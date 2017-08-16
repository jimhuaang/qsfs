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

#include <memory>
#include <utility>
#include <vector>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/HttpCommon.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"
#include "qingstor-sdk-cpp/QsErrors.h"  // for sdk QsError
#include "qingstor-sdk-cpp/Types.h"     // for sdk QsOutput
//#include "qingstor-sdk-cpp/types/ObjectPartType.h"

#include "base/LogMacros.h"
#include "client/ClientConfiguration.h"
#include "client/QSClient.h"
#include "client/QSError.h"

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
using std::string;
using std::unique_ptr;
using std::vector;

// TODO(jim): handle retryable

namespace {

ClientError<QSError> BuildQSError(QsError sdkErr, const char *exceptionName,
                                  const QsOutput &output, bool retriable) {
  return ClientError<QSError>(
      SDKErrorToQSError(sdkErr), exceptionName,
      // sdk does not provide const-qualified accessor
      SDKResponseCodeToString(const_cast<QsOutput &>(output).GetResponseCode()),
      retriable);
}

}  // namespace

// --------------------------------------------------------------------------
QSClientImpl::QSClientImpl() {
  if (!m_bucket) {
    const auto &clientConfig = ClientConfiguration::Instance();
    const auto &qsService = QSClient::GetQingStorService();
    m_bucket = unique_ptr<Bucket>(new Bucket(qsService->GetConfig(),
                                             clientConfig.GetBucket(),
                                             clientConfig.GetZone()));
  }
}

// --------------------------------------------------------------------------
GetBucketStatisticsOutcome QSClientImpl::GetBucketStatistics() const {
  GetBucketStatisticsInput input;  // dummy input
  GetBucketStatisticsOutput output;
  auto sdkErr = m_bucket->getBucketStatistics(input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return GetBucketStatisticsOutcome(std::move(output));
  } else {
    return GetBucketStatisticsOutcome(
        std::move(BuildQSError(sdkErr, "QingStorGetBucketStatistics", output,
                               SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
HeadBucketOutcome QSClientImpl::HeadBucket() const {
  HeadBucketInput input;  // dummy input
  HeadBucketOutput output;
  auto sdkErr = m_bucket->headBucket(input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return HeadBucketOutcome(std::move(output));
  } else {
    return HeadBucketOutcome(std::move(BuildQSError(
        sdkErr, "QingStorHeadBucket", output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
ListObjectsOutcome QSClientImpl::ListObjects(ListObjectsInput *input,
                                             bool *resultTruncated,
                                             uint64_t maxCount) const {
  static const char *exceptionName = "QingStorListObjects";
  if (input == nullptr) {
    return ListObjectsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null ListObjectsInput", false));
  }
  if (resultTruncated == nullptr) {
    return ListObjectsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null input of resultTruncated", false));
  }
  if (input->GetLimit() <= 0) {
    return ListObjectsOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_OBJECTS, exceptionName,
        "ListObjectsInput with negative count limit", false));
  }

  bool listAllObjects = maxCount == 0;
  uint64_t count = 0;
  bool responseTruncated = true;
  vector<ListObjectsOutput> result;
  do {
    if (!listAllObjects) {
      int remainingCount = static_cast<int>(maxCount - count);
      if (remainingCount < input->GetLimit()) input->SetLimit(remainingCount);
    }
    ListObjectsOutput output;
    auto sdkErr = m_bucket->listObjects(*input, output);
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      count += output.GetKeys().size();
      responseTruncated = !output.GetNextMarker().empty();
      if (responseTruncated) {
        input->SetMarker(output.GetNextMarker());
      }
      result.push_back(std::move(output));
    } else {
      return ListObjectsOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } while (responseTruncated && (listAllObjects || count < maxCount));
  *resultTruncated = responseTruncated;
  return ListObjectsOutcome(std::move(result));
}

// --------------------------------------------------------------------------
DeleteMultipleObjectsOutcome QSClientImpl::DeleteMultipleObjects(
    DeleteMultipleObjectsInput *input) const {
  static const char *exceptionName = "QingStorDeleteMultipleObjects";
  if (input == nullptr) {
    return DeleteMultipleObjectsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null DeleteMultiplueObjectsInput", false));
  }
  DeleteMultipleObjectsOutput output;
  auto sdkErr = m_bucket->deleteMultipleObjects(*input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    // TODO(jim): check undeleted objects
    return DeleteMultipleObjectsOutcome(std::move(output));
  } else {
    return DeleteMultipleObjectsOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
ListMultipartUploadsOutcome QSClientImpl::ListMultipartUploads(
    QingStor::ListMultipartUploadsInput *input, bool *resultTruncated,
    uint64_t maxCount) const {
  static const char *exceptionName = "QingStorListMultipartUploads";
  if (input == nullptr) {
    return ListMultipartUploadsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null ListMultipartUploadsInput", false));
  }
  if (resultTruncated == nullptr) {
    return ListMultipartUploadsOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null input of resultTruncated", false));
  }
  if (input->GetLimit() <= 0) {
    return ListMultipartUploadsOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_MULTIPART_UPLOADS, exceptionName,
        "ListMultipartUploadsInput with negative count limit", false));
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
    ListMultipartUploadsOutput output;
    auto sdkErr = m_bucket->listMultipartUploads(*input, output);
    auto responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      count += output.GetUploads().size();
      responseTruncated = !output.GetNextMarker().empty();
      if (responseTruncated) {
        input->SetMarker(output.GetNextMarker());
      }
      result.push_back(std::move(output));
    } else {
      return ListMultipartUploadsOutcome(std::move(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
    }
  } while (responseTruncated && (listAllPartUploads || count < maxCount));
  *resultTruncated = responseTruncated;
  return ListMultipartUploadsOutcome(std::move(result));
}

// --------------------------------------------------------------------------
DeleteObjectOutcome QSClientImpl::DeleteObject(const string &objKey) const {
  static const char *exceptionName = "QingStorDeleteObject";
  if (objKey.empty()) {
    return DeleteObjectOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName, "Empty ObjectKey", false));
  }
  DeleteObjectInput input;  // dummy input
  DeleteObjectOutput output;
  auto sdkErr = m_bucket->deleteObject(objKey, input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return DeleteObjectOutcome(std::move(output));
  } else {
    return DeleteObjectOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
GetObjectOutcome QSClientImpl::GetObject(const std::string &objKey,
                                         GetObjectInput *input) const {
  static const char *exceptionName = "QingStorGetObject";
  if (objKey.empty() || input == nullptr) {
    return GetObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null GetObjectInput", false));
  }
  GetObjectOutput output;
  auto sdkErr = m_bucket->getObject(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    // TODO(jim): check if get all content of object, otherwise set input status
    if (!input->GetRange().empty()) {
      if (output.GetResponseCode() == HttpResponseCode::PARTIAL_CONTENT) {
        // set input range
      }
    }
    return GetObjectOutcome(std::move(output));
  } else {
    return GetObjectOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
HeadObjectOutcome QSClientImpl::HeadObject(const string &objKey,
                                           HeadObjectInput *input) const {
  static const char *exceptionName = "QingStorHeadObject";
  if (objKey.empty() || input == nullptr) {
    return HeadObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null HeadObjectInput", false));
  }
  HeadObjectOutput output;
  auto sdkErr = m_bucket->headObject(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return HeadObjectOutcome(std::move(output));
  } else {
    return HeadObjectOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
PutObjectOutcome QSClientImpl::PutObject(const string &objKey,
                                         PutObjectInput *input) const {
  static const char *exceptionName = "QingStorPutObject";
  if (objKey.empty() || input == nullptr) {
    return PutObjectOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Empty ObjectKey or Null PutObjectInput", false));
  }
  PutObjectOutput output;
  auto sdkErr = m_bucket->putObject(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return PutObjectOutcome(std::move(output));
  } else {
    return PutObjectOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
InitiateMultipartUploadOutcome QSClientImpl::InitiateMultipartUpload(
    const string &objKey, QingStor::InitiateMultipartUploadInput *input) const {
  static const char *exceptionName = "QingStorInitiateMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return InitiateMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null InitiateMultipartUploadInput", false));
  }
  InitiateMultipartUploadOutput output;
  auto sdkErr = m_bucket->initiateMultipartUpload(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return InitiateMultipartUploadOutcome(std::move(output));
  } else {
    return InitiateMultipartUploadOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
UploadMultipartOutcome QSClientImpl::UploadMultipart(
    const string &objKey, UploadMultipartInput *input) const {
  static const char *exceptionName = "QingStorUploadMultipart";
  if (objKey.empty() || input == nullptr) {
    return UploadMultipartOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null UploadMultipartInput", false));
  }
  UploadMultipartOutput output;
  auto sdkErr = m_bucket->uploadMultipart(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return UploadMultipartOutcome(std::move(output));
  } else {
    return UploadMultipartOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
CompleteMultipartUploadOutcome QSClientImpl::CompleteMultipartUpload(
    const string &objKey, CompleteMultipartUploadInput *input) const {
  static const char *exceptionName = "QingStorCompleteMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return CompleteMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null CompleteMutlipartUploadInput", false));
  }
  CompleteMultipartUploadOutput output;
  auto sdkErr = m_bucket->completeMultipartUpload(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return CompleteMultipartUploadOutcome(std::move(output));
  } else {
    return CompleteMultipartUploadOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
AbortMultipartUploadOutcome QSClientImpl::AbortMultipartUpload(
    const string &objKey, AbortMultipartUploadInput *input) const {
  static const char *exceptionName = "QingStorAbortMultipartUpload";
  if (objKey.empty() || input == nullptr) {
    return AbortMultipartUploadOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null AbortMultipartUploadInput", false));
  }
  AbortMultipartUploadOutput output;
  auto sdkErr = m_bucket->abortMultipartUpload(objKey, *input, output);
  auto responseCode = output.GetResponseCode();
  if (SDKResponseSuccess(sdkErr, responseCode)) {
    return AbortMultipartUploadOutcome(std::move(output));
  } else {
    return AbortMultipartUploadOutcome(std::move(BuildQSError(
        sdkErr, exceptionName, output, SDKShouldRetry(responseCode))));
  }
}

// --------------------------------------------------------------------------
ListMultipartOutcome QSClientImpl::ListMultipart(
    const string &objKey, QingStor::ListMultipartInput *input,
    bool *resultTruncated, uint64_t maxCount) const {
  static const char *exceptionName = "QingStorListMultipart";
  if (objKey.empty() || input == nullptr) {
    return ListMultipartOutcome(ClientError<QSError>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null ListMultipartInput", false));
  }
  if (resultTruncated == nullptr) {
    return ListMultipartOutcome(
        ClientError<QSError>(QSError::PARAMETER_MISSING, exceptionName,
                             "Null input of resultTruncated", false));
  }
  if (input->GetLimit() <= 0) {
    return ListMultipartOutcome(ClientError<QSError>(
        QSError::NO_SUCH_LIST_MULTIPART, exceptionName,
        "ListMultipartInput with negative count limit", false));
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
    ListMultipartOutput output;
    auto sdkErr = m_bucket->listMultipart(objKey, *input, output);
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
  } while (responseTruncated && (listAllParts || count < maxCount));
  *resultTruncated = responseTruncated;
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
