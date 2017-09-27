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
#include "client/Credentials.h"
#include "client/Protocol.h"
#include "client/RetryStrategy.h"
#include "client/URI.h"
#include "client/Zone.h"
#include "filesystem/Configure.h"
#include "filesystem/Options.h"

namespace QS {

namespace Client {

using QS::Exception::QSException;
using QS::FileSystem::Configure::GetClientDefaultPoolSize;
using QS::FileSystem::Configure::GetDefaultLogDirectory;
using QS::FileSystem::Configure::GetDefineFileMode;
using QS::FileSystem::Configure::GetQSConnectionDefaultRetries;
using QS::FileSystem::Configure::GetQingStorSDKLogFileName;
using QS::FileSystem::Configure::GetTransactionDefaultTimeDuration;
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
  if(name.empty()) {
    return level;
  }

  auto name_lowercase = QS::StringUtils::ToLower(name);
  if(name_lowercase == "debug"){
    level = ClientLogLevel::Debug;
  } else if (name_lowercase == "info"){
    level = ClientLogLevel::Info;
  }else if (name_lowercase == "error"){
    level = ClientLogLevel::Error;
  } else if(name_lowercase == "fatal"){
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
      m_zone(QS::Client::GetDefaultZone()),
      m_host(Http::GetDefaultHost()),
      m_protocol(QS::Client::Http::GetDefaultProtocol()),
      m_port(Http::GetDefaultPort(m_protocol)),
      m_connectionRetries(GetQSConnectionDefaultRetries()),
      m_additionalUserAgent(std::string()),
      m_logLevel(ClientLogLevel::Warn),
      m_logFile(GetDefaultLogDirectory() + GetQingStorSDKLogFileName()),
      m_transactionRetries(Retry::DefaultMaxRetries),
      m_transactionTimeDuration(GetTransactionDefaultTimeDuration()),
      m_clientPoolSize(GetClientDefaultPoolSize()) {}

ClientConfiguration::ClientConfiguration(const CredentialsProvider &provider)
    : ClientConfiguration(provider.GetCredentials()) {}

void ClientConfiguration::InitializeByOptions() {
  const auto &options = QS::FileSystem::Options::Instance();
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
    throw(string("File to create log file for sdk : ") + strerror(errno));
  }

  m_transactionRetries = options.GetRetries();
}

}  // namespace Client
}  // namespace QS
