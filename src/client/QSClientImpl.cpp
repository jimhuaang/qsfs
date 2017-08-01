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

#include <utility>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsErrors.h"

#include "base/LogMacros.h"
#include "client/ClientConfiguration.h"
#include "client/QSClient.h"
#include "client/QSError.h"

namespace QS {

namespace Client {

using QingStor::Bucket;
using QingStor::HeadBucketInput;
using QingStor::HeadBucketOutput;
using std::unique_ptr;

QSClientImpl::QSClientImpl() {
  if (!m_bucket) {
    const auto &clientConfig = ClientConfiguration::Instance();
    const auto &qsService = QSClient::GetQingStorService();
    m_bucket = unique_ptr<Bucket>(new Bucket(qsService->GetConfig(),
                                             clientConfig.GetBucket(),
                                             clientConfig.GetZone()));
  }
}

HeadBucketOutcome QSClientImpl::HeadBucket() const {
  HeadBucketInput input;
  HeadBucketOutput output;
  auto qsErr = m_bucket->headBucket(input, output);
  if (SDKRequestSuccess(qsErr)) {
    return HeadBucketOutcome(output);
  } else {
    ClientError<QSError> err(SDKErrorToQSError(qsErr), "QingStorHeadBucket",
                             SDKResponseToString(output.GetResponseCode()),
                             false);
    return HeadBucketOutcome(std::move(err));
  }
}

void QSClientImpl::HeadObject() {
  // TODO(jim):
}

void QSClientImpl::SetBucket(unique_ptr<QingStor::Bucket> bucket) {
  assert(bucket);
  FatalIf(!bucket, "Setting a NULL bucket!");
  m_bucket = std::move(bucket);
}

}  // namespace Client
}  // namespace QS
