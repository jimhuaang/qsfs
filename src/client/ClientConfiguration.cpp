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

#include "client/ClientConfiguration.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>     // for fopen
#include <string.h>    // for strerr
#include <sys/stat.h>  // for chmod

#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <utility>

#include "base/Exception.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "client/Credentials.h"
#include "client/Protocol.h"
#include "client/URI.h"
#include "configure/Default.h"
#include "configure/Options.h"
#include "data/Size.h"

namespace QS {

namespace Client {

using QS::Exception::QSException;
using QS::Configure::Default::GetClientDefaultPoolSize;
using QS::Configure::Default::GetDefaultLogDirectory;
using QS::Configure::Default::GetDefaultMaxRetries;
using QS::Configure::Default::GetDefaultParallelTransfers;
using QS::Configure::Default::GetDefaultTransferBufSize;
using QS::Configure::Default::GetDefaultHostName;
using QS::Configure::Default::GetDefaultPort;
using QS::Configure::Default::GetDefaultProtocolName;
using QS::Configure::Default::GetDefaultZone;
using QS::Configure::Default::GetDefineFileMode;
using QS::Configure::Default::GetQSConnectionDefaultRetries;
using QS::Configure::Default::GetQingStorSDKLogFileName;
using QS::Configure::Default::GetTransactionDefaultTimeDuration;
using QS::StringUtils::FormatPath;
using std::call_once;
using std::string;
using std::unique_ptr;

string GetClientLogLevelName(ClientLogLevel level) {
  string name;
  switch (level) {
    case ClientLogLevel::Debug:
      name = "debug";
      break;
    case ClientLogLevel::Info:
      name = "info";
      break;
    case ClientLogLevel::Warn:
      name = "warning";
      break;
    case ClientLogLevel::Error:
      name = "error";
      break;
    case ClientLogLevel::Fatal:
      name = "fatal";
      break;
    default:
      break;
  }
  return name;
}

ClientLogLevel GetClientLogLevelByName(const string &name) {
  ClientLogLevel level = ClientLogLevel::Warn;
  if (name.empty()) {
    return level;
  }

  auto name_lowercase = QS::StringUtils::ToLower(name);
  if (name_lowercase == "debug") {
    level = ClientLogLevel::Debug;
  } else if (name_lowercase == "info") {
    level = ClientLogLevel::Info;
  } else if (name_lowercase == "error") {
    level = ClientLogLevel::Error;
  } else if (name_lowercase == "fatal") {
    level = ClientLogLevel::Fatal;
  }

  return level;
}

static unique_ptr<ClientConfiguration> clientConfigInstance(nullptr);
static std::once_flag clientConfigFlag;

void InitializeClientConfiguration(unique_ptr<ClientConfiguration> config) {
  assert(config);
  call_once(clientConfigFlag,
            [&config] { clientConfigInstance = std::move(config); });
}

ClientConfiguration &ClientConfiguration::Instance() {
  call_once(clientConfigFlag, [] {
    clientConfigInstance =
        unique_ptr<ClientConfiguration>(new ClientConfiguration);
  });
  return *clientConfigInstance.get();
}

ClientConfiguration::ClientConfiguration(const Credentials &credentials)
    : m_accessKeyId(credentials.GetAccessKeyId()),
      m_secretKey(credentials.GetSecretKey()),
      m_zone(GetDefaultZone()),
      m_host(QS::Client::Http::StringToHost(GetDefaultHostName())),
      m_protocol(QS::Client::Http::StringToProtocol(GetDefaultProtocolName())),
      m_port(GetDefaultPort(GetDefaultProtocolName())),
      m_connectionRetries(GetQSConnectionDefaultRetries()),
      m_additionalUserAgent(std::string()),
      m_logLevel(ClientLogLevel::Warn),
      m_logFile(GetDefaultLogDirectory() + GetQingStorSDKLogFileName()),
      m_transactionRetries(GetDefaultMaxRetries()),
      m_transactionTimeDuration(GetTransactionDefaultTimeDuration()),
      m_clientPoolSize(GetClientDefaultPoolSize()),
      m_parallelTransfers(GetDefaultParallelTransfers()),
      m_transferBufferSizeInMB(GetDefaultTransferBufSize() /
                                QS::Data::Size::MB1) {}

ClientConfiguration::ClientConfiguration(const CredentialsProvider &provider)
    : ClientConfiguration(provider.GetCredentials()) {}

void ClientConfiguration::InitializeByOptions() {
  const auto &options = QS::Configure::Options::Instance();
  m_bucket = options.GetBucket();
  m_zone = options.GetZone();
  m_host = Http::StringToHost(options.GetHost());
  m_protocol = Http::StringToProtocol(options.GetProtocol());
  m_port = options.GetPort();
  m_additionalUserAgent = options.GetAdditionalAgent();
  m_logLevel = static_cast<ClientLogLevel>(options.GetLogLevel());
  if (options.IsDebug()) {
    m_logLevel = ClientLogLevel::Debug;
  }

  if (!QS::Utils::CreateDirectoryIfNotExistsNoLog(options.GetLogDirectory())) {
    throw QSException(string("Unable to create log directory : ") +
                      strerror(errno) + " " +
                      FormatPath(options.GetLogDirectory()));
  }

  m_logFile = options.GetLogDirectory() + GetQingStorSDKLogFileName();
  FILE *log = nullptr;
  if (options.IsClearLogDir()) {
    log = fopen(m_logFile.c_str(), "wb");
  } else {
    log = fopen(m_logFile.c_str(), "ab");
  }
  if (log != nullptr) {
    fclose(log);
    chmod(m_logFile.c_str(), GetDefineFileMode());
  } else {
    throw(string("Fail to create log file for sdk : ") + strerror(errno) + " " +
          FormatPath(m_logFile));
  }

  m_transactionRetries = options.GetRetries();
  m_transactionTimeDuration = options.GetRequestTimeOut();
  m_clientPoolSize = options.GetClientPoolSize();
  m_parallelTransfers = options.GetParallelTransfers();
  m_transferBufferSizeInMB = options.GetTransferBufferSizeInMB();
}

}  // namespace Client
}  // namespace QS
