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

#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <utility>

#include "base/Utils.h"
#include "client/Credentials.h"
#include "client/Protocol.h"
#include "client/URI.h"
#include "client/Zone.h"
#include "filesystem/Configure.h"
#include "filesystem/Options.h"

namespace QS {

namespace Client {

using QS::Utils::EnumHash;
using QS::Utils::StringHash;
using std::call_once;
using std::string;
using std::unique_ptr;
using std::unordered_map;

const static int CONNECTION_DEFAULT_RETRIES = 3;

const string &GetClientLogLevelName(ClientLogLevel level) {
  static unordered_map<ClientLogLevel, string, EnumHash> logLevelNames = {
      {ClientLogLevel::Debug, "debug"},
      {ClientLogLevel::Info, "info"},
      {ClientLogLevel::Warn, "warning"},
      {ClientLogLevel::Error, "error"},
      {ClientLogLevel::Fatal, "fatal"}};
  return logLevelNames[level];
}

ClientLogLevel GetClientLogLevelByName(const string &name) {
  static unordered_map<string, ClientLogLevel, StringHash> nameLogLevels = {
      {"debug", ClientLogLevel::Debug},
      {"Debug", ClientLogLevel::Debug},
      {"DEBUG", ClientLogLevel::Debug},
      {"info", ClientLogLevel::Info},
      {"Info", ClientLogLevel::Info},
      {"INFO", ClientLogLevel::Info},
      {"warn", ClientLogLevel::Warn},
      {"Warn", ClientLogLevel::Warn},
      {"WARN", ClientLogLevel::Warn},
      {"warning", ClientLogLevel::Warn},
      {"Warning", ClientLogLevel::Warn},
      {"WARNING", ClientLogLevel::Warn},
      {"error", ClientLogLevel::Error},
      {"Error", ClientLogLevel::Error},
      {"ERROR", ClientLogLevel::Error},
      {"fatal", ClientLogLevel::Fatal},
      {"Fatal", ClientLogLevel::Fatal},
      {"FATAL", ClientLogLevel::Fatal}
      // Add other entries here
  };
  auto it = nameLogLevels.find(name);
  return it != nameLogLevels.end() ? it->second : ClientLogLevel::Warn;
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

ClientConfiguration::ClientConfiguration(
    const Credentials &credentials =
        GetCredentialsProviderInstance().GetCredentials())
    : m_accessKeyId(credentials.GetAccessKeyId()),
      m_secretKey(credentials.GetSecretKey()),
      m_location(QS::Client::GetDefaultZone()),
      m_host(Http::GetDefaultHost()),
      m_protocol(QS::Client::Http::GetDefaultProtocol()),
      m_port(Http::GetDefaultPort(m_protocol)),
      m_connectionRetries(CONNECTION_DEFAULT_RETRIES),
      m_additionalUserAgent(std::string()),
      m_logLevel(ClientLogLevel::Warn) {}

ClientConfiguration::ClientConfiguration(const CredentialsProvider &provider)
    : ClientConfiguration(provider.GetCredentials()) {}

void ClientConfiguration::InitializeByOptions(){
  const auto& options = QS::FileSystem::Options::Instance();
  m_location = options.GetZone();
  m_host = Http::StringToHost(options.GetHost());
  m_protocol = Http::StringToProtocol(options.GetProtocol());
  m_port = options.GetPort();
  m_connectionRetries = CONNECTION_DEFAULT_RETRIES;
  m_additionalUserAgent = options.GetAdditionalAgent();
  m_logLevel = static_cast<ClientLogLevel>(options.GetLogLevel());
  if(options.IsDebug()){
    m_logLevel = ClientLogLevel::Debug;
  }
}

}  // namespace Client
}  // namespace QS
