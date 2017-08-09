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

#include "client/QSClient.h"

#include <assert.h>
#include <stdint.h>  // for uint64_t

#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <utility>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
#include "client/Constants.h"
#include "client/Protocol.h"
#include "client/QSClientImpl.h"
#include "client/QSClientUtils.h"
#include "client/QSError.h"
#include "client/URI.h"
#include "filesystem/Drive.h"

namespace QS {

namespace Client {

using QingStor::Bucket;
using QingStor::ListObjectsInput;
using QingStor::QingStorService;
using QingStor::QsConfig;  // sdk config
using QS::StringUtils::LTrim;
using QS::Utils::AppendPathDelim;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

static std::once_flag onceFlagGetClientImpl;
static std::once_flag onceFlagStartService;
unique_ptr<QingStor::QingStorService> QSClient::m_qingStorService = nullptr;

// --------------------------------------------------------------------------
QSClient::QSClient() : Client() {
  StartQSService();
  InitializeClientImpl();
}

// --------------------------------------------------------------------------
QSClient::~QSClient() { CloseQSService(); }

// --------------------------------------------------------------------------
bool QSClient::Connect() {
  auto outcome = GetQSClientImpl()->HeadBucket();
  /*   auto err =
        ReadDirectory("/");  // TODO(jim): for test only, remove */
  if (outcome.IsSuccess()) {
    auto receivedHandler = [](const ClientError<QSError> &err) {
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    };
    // Build up the root level of directory tree asynchornizely.
    GetExecutor()->SubmitAsync(receivedHandler,
                               [this] { return ReadDirectory("/"); });
    return true;
  } else {
    DebugError(GetMessageForQSError(outcome.GetError()));
    return false;
  }
}

// --------------------------------------------------------------------------
bool QSClient::DisConnect() {
  // TODO(jim):
  return true;
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::ReadDirectory(const string &dirPath) {
  auto &drive = QS::FileSystem::Drive::Instance();
  ListObjectsInput listObjInput;
  listObjInput.SetLimit(Constants::BucketListObjectsCountLimit);
  listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
  if(!QS::Utils::IsRootDirectory(dirPath)){
    string prefix = AppendPathDelim(LTrim(dirPath, '/'));
    listObjInput.SetPrefix(prefix);
  }
  bool resultTruncated = false;
  // Set maxCount for a single list operation.
  // This will request for ListObjects seperately, so we can construct
  // directory tree gradually. This will be helpful for the performance
  // if there are a huge number of objects to list.
  uint64_t maxCount =
      static_cast<uint64_t>(Constants::BucketListObjectsCountLimit);
  do {
    auto outcome = GetQSClientImpl()->ListObjects(&listObjInput,
                                                  &resultTruncated, maxCount);
    if (outcome.IsSuccess()) {
      for (auto &listObjOutput : outcome.GetResult()) {
        auto fileMetaDatas =
            QSClientUtils::ListObjectsOutputToFileMetaDatas(listObjOutput);
        drive.GrowDirectoryTree(std::move(fileMetaDatas));
      }
    } else {
      return outcome.GetError();
    }
  } while (resultTruncated);
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteFile(const string &filePath) {
  // TODO(jim):
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteDirectory(const string &dirPath) {
  // TODO(jim):
  // call list object
  // and recursive delete all object under
  // can we use delete multiple objects
  // maybe can call deleteMultipleObjects
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
const unique_ptr<QingStorService> &QSClient::GetQingStorService() {
  StartQSService();
  return m_qingStorService;
}

// --------------------------------------------------------------------------
const shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() const {
  return const_cast<QSClient *>(this)->GetQSClientImpl();
}

// --------------------------------------------------------------------------
shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() {
  std::call_once(onceFlagGetClientImpl, [this] {
    auto r = GetClientImpl();
    assert(r);
    FatalIf(!r, "QSClient is initialized with null QSClientImpl");
    auto p = dynamic_cast<QSClientImpl *>(r.get());
    m_qsClientImpl = std::move(shared_ptr<QSClientImpl>(r, p));
  });
  return m_qsClientImpl;
}

// --------------------------------------------------------------------------
void QSClient::StartQSService() {
  std::call_once(onceFlagStartService, [] {
    const auto &clientConfig = ClientConfiguration::Instance();
    // Must set sdk log at beginning, otherwise sdk will broken due to
    // uninitialization of plog sdk depended on.
    QingStorService::initService(clientConfig.GetClientLogFile());

    static QsConfig sdkConfig(clientConfig.GetAccessKeyId(),
                              clientConfig.GetSecretKey());
    sdkConfig.m_LogLevel =
        GetClientLogLevelName(clientConfig.GetClientLogLevel());
    sdkConfig.m_AdditionalUserAgent = clientConfig.GetAdditionalAgent();
    sdkConfig.m_Host = Http::HostToString(clientConfig.GetHost());
    sdkConfig.m_Protocol = Http::ProtocolToString(clientConfig.GetProtocol());
    sdkConfig.m_Port = clientConfig.GetPort();
    sdkConfig.m_ConnectionRetries = clientConfig.GetConnectionRetries();
    m_qingStorService =
        unique_ptr<QingStorService>(new QingStorService(sdkConfig));
  });
}

// --------------------------------------------------------------------------
void QSClient::CloseQSService() { QingStorService::shutdownService(); }

// --------------------------------------------------------------------------
void QSClient::InitializeClientImpl() {
  const auto &clientConfig = ClientConfiguration::Instance();
  if (GetQSClientImpl()->GetBucket()) return;
  GetQSClientImpl()->SetBucket(unique_ptr<Bucket>(
      new Bucket(m_qingStorService->GetConfig(), clientConfig.GetBucket(),
                 clientConfig.GetZone())));
}

}  // namespace QSClient
}  // namespace QS
