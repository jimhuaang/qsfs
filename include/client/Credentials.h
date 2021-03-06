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

#ifndef INCLUDE_CLIENT_CREDENTIALS_H_
#define INCLUDE_CLIENT_CREDENTIALS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/HashUtils.h"

namespace QS {

namespace Client {

class CredentialsProvider;

void InitializeCredentialsProvider(
    std::unique_ptr<CredentialsProvider> provider);

CredentialsProvider &GetCredentialsProviderInstance();

class Credentials {
 public:
  Credentials() = default;

  Credentials(const std::string &accessKeyId, const std::string &secretKey)
      : m_accessKeyId(accessKeyId), m_secretKey(secretKey) {}

  Credentials(Credentials &&) = default;
  Credentials(const Credentials &) = default;
  Credentials &operator=(Credentials &&) = default;
  Credentials &operator=(const Credentials &) = default;
  ~Credentials() = default;

 public:
  const std::string &GetAccessKeyId() const { return m_accessKeyId; }
  const std::string &GetSecretKey() const { return m_secretKey; }

  void SetAccessKeyId(const std::string &accessKeyId) {
    m_accessKeyId = accessKeyId;
  }
  void SetSecretKey(const std::string &secretKey) { m_secretKey = secretKey; }

 private:
  std::string m_accessKeyId;
  std::string m_secretKey;
};

class CredentialsProvider {
 public:
  virtual Credentials GetCredentials() const = 0;
  virtual Credentials GetCredentials(const std::string &bucket) const = 0;
  virtual ~CredentialsProvider() = default;
};

// For public bucket
class AnonymousCredentialsProvider : public CredentialsProvider {
  Credentials GetCredentials() const override {
    return Credentials(std::string(), std::string());
  }

  Credentials GetCredentials(const std::string &bucket) const {
    return GetCredentials();
  }
};

class DefaultCredentialsProvider : public CredentialsProvider {
 public:
  DefaultCredentialsProvider(const std::string &accessKeyId,
                               const std::string &secretKey)
      : m_defaultAccessKeyId(accessKeyId), m_defaultSecretKey(secretKey) {}

  explicit DefaultCredentialsProvider(const std::string &credentialFile);

  Credentials GetCredentials() const override;
  Credentials GetCredentials(const std::string &bucket) const override;

  bool HasDefaultKey() const {
    return (!m_defaultAccessKeyId.empty()) && (!m_defaultSecretKey.empty());
  }

 private:
  // Read credentials file
  //
  // @param  : credentials file path
  // @return : a pair of {true, ""} or {false, message}
  //
  // Credentials file format: [bucket:]AccessKeyId:SecretKey
  // Support for per bucket credentials;
  // Set default key pair by not providing bucket name;
  // Only allow to have one default key pair, but not required to.
  //
  // Comment line is beginning with #;
  // Empty lines are ignored;
  // Uncommented lines without the ":" character are flaged as an error,
  // so are lines with space or tabs and lines starting with bracket "[".
  std::pair<bool, std::string> ReadCredentialsFile(const std::string &file);

  void SetDefaultKey(const std::string &keyId, const std::string &key) {
    m_defaultAccessKeyId = keyId;
    m_defaultSecretKey = key;
  }

 private:
  using KeyIdToKeyPair = std::pair<std::string, std::string>;
  using BucketToKeyPairMap = std::unordered_map<std::string, KeyIdToKeyPair,
                                                QS::HashUtils::StringHash>;

  std::string m_credentialsFile;
  std::string m_defaultAccessKeyId;
  std::string m_defaultSecretKey;
  BucketToKeyPairMap m_bucketMap;
};

}  // namespace Client
}  // namespace QS


#endif  // INCLUDE_CLIENT_CREDENTIALS_H_
