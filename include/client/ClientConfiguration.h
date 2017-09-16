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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_CLIENTCONFIGURATION_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_CLIENTCONFIGURATION_H_  // NOLINT

#include <stdint.h>

#include <memory>
#include <string>

#include "client/Credentials.h"
#include "client/Protocol.h"
#include "client/URI.h"

// Declare in global namespace before class ClientConfiguration, since friend
// declarations can only introduce names in the surrounding namespace.
extern void ClientConfigurationInitializer();

namespace QS {

namespace Client {

class QSClient;

enum class ClientLogLevel : int {  // SDK log level
  Debug = -1,
  Info = 0,
  Warn = 1,
  Error = 2,
  Fatal = 3
};

std::string GetClientLogLevelName(ClientLogLevel level);
ClientLogLevel GetClientLogLevelByName(const std::string& name);

class ClientConfiguration;
void InitializeClientConfiguration(std::unique_ptr<ClientConfiguration> config);

class ClientConfiguration {
 public:
  static ClientConfiguration& Instance();

 public:
  explicit ClientConfiguration(
      const Credentials& credentials =
          GetCredentialsProviderInstance().GetCredentials());
  explicit ClientConfiguration(const CredentialsProvider& provider);

  ClientConfiguration(const ClientConfiguration&) = default;
  ClientConfiguration(ClientConfiguration&&) = default;
  ClientConfiguration& operator=(const ClientConfiguration&) = default;
  ClientConfiguration& operator=(ClientConfiguration&&) = default;

 public:
  // accessor
  const std::string& GetBucket() const { return m_bucket; }
  const std::string& GetZone() const { return m_zone; }
  Http::Host GetHost() const { return m_host; }
  Http::Protocol GetProtocol() const { return m_protocol; }
  uint16_t GetPort() const { return m_port; }
  int GetConnectionRetries() const { return m_connectionRetries; }
  const std::string& GetAdditionalAgent() const {
    return m_additionalUserAgent;
  }
  ClientLogLevel GetClientLogLevel() const { return m_logLevel; }
  const std::string& GetClientLogFile() const { return m_logFile; }
  uint16_t GetTransactionRetries() const { return m_transactionRetries; }
  uint16_t GetPoolSize() const { return m_clientPoolSize; }

 private:
  const std::string& GetAccessKeyId() const { return m_accessKeyId; }
  const std::string& GetSecretKey() const { return m_secretKey; }
  void InitializeByOptions();
  friend void ::ClientConfigurationInitializer();
  friend class QSClient;  // for QSClient::StartQSService

 private:
  std::string m_accessKeyId;
  std::string m_secretKey;
  std::string m_bucket;
  std::string m_zone;  // zone or region
  Http::Host m_host;
  Http::Protocol m_protocol;
  uint16_t m_port;
  int m_connectionRetries;
  std::string m_additionalUserAgent;
  ClientLogLevel m_logLevel;
  std::string m_logFile;  // log file path

  uint16_t m_transactionRetries;  // retry times when transaction fails
  uint16_t m_clientPoolSize;      // pool size of client
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_CLIENTCONFIGURATION_H_
