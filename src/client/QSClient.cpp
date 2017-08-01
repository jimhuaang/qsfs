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

#include <mutex>  // NOLINT
#include <utility>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"

#include "base/LogMacros.h"
#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
#include "client/Protocol.h"
#include "client/QSClientImpl.h"
#include "client/URI.h"

namespace QS {

namespace Client {

using QingStor::Bucket;
using QingStor::QingStorService;
using QingStor::QsConfig;  // sdk config
using std::shared_ptr;
using std::unique_ptr;

static std::once_flag onceFlag;
unique_ptr<QingStor::QingStorService> QSClient::m_qingStorService = nullptr;

QSClient::QSClient() : Client() {
  StartQSService();
  InitializeClientImpl();
}

QSClient::~QSClient() { CloseQSService(); }

bool QSClient::Connect() { return true; }
bool QSClient::DisConnect() { return true; }

std::shared_ptr<QSClientImpl> QSClient::GetQSClientImpl() {
  std::call_once(onceFlag, [this] {
    auto r = GetClientImpl();
    assert(r);
    FatalIf(!r, "QSClient is initialized with null QSClientImpl");
    auto p = dynamic_cast<QSClientImpl *>(r.get());
    m_qsClientImpl = std::move(shared_ptr<QSClientImpl>(r, p));
  });
  return m_qsClientImpl;
}

void QSClient::StartQSService() {
  const auto &clientConfig = ClientConfiguration::Instance();
  QingStorService::initService(clientConfig.GetClientLogFile());
  QsConfig sdkConfig(clientConfig.GetAccessKeyId(),
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
}

void QSClient::CloseQSService() { QingStorService::shutdownService(); }

void QSClient::InitializeClientImpl() {
  const auto &clientConfig = ClientConfiguration::Instance();
  if(GetQSClientImpl()->GetBucket()) return;
  GetQSClientImpl()->SetBucket(unique_ptr<Bucket>(
      new Bucket(m_qingStorService->GetConfig(), clientConfig.GetBucket(),
                 clientConfig.GetZone())));
}

}  // namespace QSClient
}  // namespace QS
