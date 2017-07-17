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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_QSCREDENTIALS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_QSCREDENTIALS_H_  // NOLINT

#include <string>
#include <unordered_map>
#include <utility>

#include "base/Exception.h"
#include "base/Utils.h"

namespace QS {

namespace Client {

class QSCredentials {
 public:
  QSCredentials() = default;

  QSCredentials(const std::string &accessKeyId, const std::string &secretKey)
      : m_accessKeyId(accessKeyId), m_secretKey(secretKey) {}

  QSCredentials(QSCredentials &&) = default;
  QSCredentials(const QSCredentials &) = default;
  QSCredentials &operator=(QSCredentials &&) = default;
  QSCredentials &operator=(const QSCredentials &) = default;
  ~QSCredentials() = default;

 public:
  const std::string &GetAccessKeyId() const { return m_accessKeyId; }
  const std::string &GetSecretKey() const { return m_secretKey; }

  void SetAccessKeyId(const std::string &accessKeyId) {
    m_accessKeyId = accessKeyId;
  }
  void SetAccessKeyId(const char *accessKeyId) { m_accessKeyId = accessKeyId; }
  void SetSecretKey(const std::string &secretKey) { m_secretKey = secretKey; }
  void SetSecretKey(const char *secretKey) { m_secretKey = secretKey; }

 private:
  std::string m_accessKeyId;
  std::string m_secretKey;
};

class QSCredentialsProvider {
 public:
  virtual QSCredentials GetCredentials() const = 0;
  virtual QSCredentials GetCredentials(const std::string &bucket) const = 0;
  virtual ~QSCredentialsProvider() = default;
};

// For public bucket
class AnonymousQSCredentialsProvider : public QSCredentialsProvider {
  QSCredentials GetCredentials() const override {
    return QSCredentials(std::string(), std::string());
  }

  QSCredentials GetCredentials(const std::string &bucket) const {
    return GetCredentials();
  }
};

class DefaultQSCredentialsProvider : public QSCredentialsProvider {
 public:
  DefaultQSCredentialsProvider(const std::string &accessKeyId,
                               const std::string &secretKey)
      : m_defaultAccessKeyId(accessKeyId), m_defaultSecretKey(secretKey) {}

  explicit DefaultQSCredentialsProvider(const std::string &credentialFile);

  QSCredentials GetCredentials() const override {
    if (!HasDefaultKey()) {
      throw QS::Exception::QSException(
          "Fail to fetch default credentials which is not existing.");
    }

    return QSCredentials(m_defaultAccessKeyId, m_defaultSecretKey);
  }

  QSCredentials GetCredentials(const std::string &bucket) const override {
    auto it = m_bucketMap.find(bucket);
    if (it == m_bucketMap.end()) {
      throw QS::Exception::QSException(
          "Fail to fetch access key for bucket " + bucket +
          "which is not found in credentials file " + m_credentialsFile + ".");
    }
    return QSCredentials(it->second.first, it->second.second);
  }

  bool HasDefaultKey() const {
    return (!m_defaultAccessKeyId.empty()) && (!m_defaultSecretKey.empty());
  }

 private:
  // Returned pair with first member to denote if read credentials successfully,
  // if fail to read credentials, second member contain error messages.
  //
  // Credentials file format: [bucket:]AccessKeyId:SecretKey
  // Support for per bucket credentials.
  // To set default key pair by not providing bucket name.
  // Only allow to have one default key pair, but not required to.
  //
  // Comment line beginning with #. Empty line are ignored.
  // Uncommented lines without the ":" character are flaged as an error,
  // so are lines with space or tabs and lines starting with bracket "[".
  std::pair<bool, std::string> ReadCredentialsFile(const std::string &file);

  void SetDefaultKey(const std::string &keyId, const std::string &key);

 private:
  using KeyIdToKeyPair = std::pair<std::string, std::string>;
  using BucketToKeyPairMap =
      std::unordered_map<std::string, KeyIdToKeyPair, QS::Utils::StringHash>;

  std::string m_credentialsFile;
  std::string m_defaultAccessKeyId;
  std::string m_defaultSecretKey;
  BucketToKeyPairMap m_bucketMap;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_QSCREDENTIALS_H_
