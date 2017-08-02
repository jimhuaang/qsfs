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
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"
#include "qingstor-sdk-cpp/QsErrors.h"  // for sdk QsError
#include "qingstor-sdk-cpp/Types.h"     // for sdk QsOutput

#include "base/LogMacros.h"
#include "client/ClientConfiguration.h"
#include "client/QSClient.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

using QingStor::Bucket;
using QingStor::HeadBucketInput;
using QingStor::HeadBucketOutput;
using QingStor::ListObjectsInput;
using QingStor::ListObjectsOutput;
using QingStor::QsOutput;
using std::unique_ptr;
using std::vector;

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
HeadBucketOutcome QSClientImpl::HeadBucket() const {
  HeadBucketInput input;
  HeadBucketOutput output;
  auto sdkErr = m_bucket->headBucket(input, output);
  if (SDKRequestSuccess(sdkErr)) {
    return HeadBucketOutcome(std::move(output));
  } else {
    return HeadBucketOutcome(
        std::move(BuildQSError(sdkErr, "QingStorHeadBucket", output, false)));
  }
}

// --------------------------------------------------------------------------
ListObjectsOutcome QSClientImpl::ListObjects(ListObjectsInput *input,
                                             bool *resultTruncated,
                                             uint64_t maxCount) const {
  static const char *exceptionName = "QingStorListObjects";
  if (input == nullptr) {
    return ListObjectsOutcome(
        ClientError<QSError>(QSError::NO_SUCH_LIST_OBJECTS, exceptionName,
                             "Null ListObjectsInput", false));
  }
  if (resultTruncated == nullptr) {
    return ListObjectsOutcome(
        ClientError<QSError>(QSError::NO_SUCH_LIST_OBJECTS, exceptionName,
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
    if (SDKRequestSuccess(sdkErr)) {
      count += output.GetKeys().size();
      responseTruncated = !output.GetNextMarker().empty();
      input->SetMarker(output.GetNextMarker());
      result.push_back(std::move(output));
    } else {
      return ListObjectsOutcome(
          std::move(BuildQSError(sdkErr, exceptionName, output, false)));
    }
  } while (responseTruncated && (listAllObjects || count < maxCount));
  *resultTruncated = responseTruncated;
  return ListObjectsOutcome(std::move(result));
}

// --------------------------------------------------------------------------
void QSClientImpl::HeadObject() {
  // TODO(jim):
}

// --------------------------------------------------------------------------
void QSClientImpl::SetBucket(unique_ptr<QingStor::Bucket> bucket) {
  assert(bucket);
  FatalIf(!bucket, "Setting a NULL bucket!");
  m_bucket = std::move(bucket);
}

}  // namespace Client
}  // namespace QS
