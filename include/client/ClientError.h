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

#ifndef _QSFS_FUSE_INCLUDE_CLIENT_CLIENTERROR_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_CLIENT_CLIENTERROR_H_  // NOLINT

#include <string>

namespace QS {

namespace Client {

template <typename ERROR_TYPE>
class ClientError {
 public:
  ClientError() : m_isRetryable(false) {}
  ClientError(ERROR_TYPE errorType, const std::string &exceptionName,
        const std::string &errorMsg, bool isRetryable)
      : m_errorType(errorType),
        m_exceptionName(exceptionName),
        m_message(errorMsg),
        m_isRetryable(isRetryable) {}
  ClientError(ERROR_TYPE errorType, bool isRetryable)
      : ClientError(errorType, "", "", isRetryable) {}

 public:
  const ERROR_TYPE GetErrorType() const { return m_errorType; }
  const std::string &GetExceptionName() const { return m_exceptionName; }
  const std::string &GetMessage() const { return m_message; }
  bool ShouldRetry() const { return m_isRetryable; }

  void SetExceptionName(const std::string &exceptionName) {
    m_exceptionName = exceptionName;
  }
  void SetMessage(const std::string &message) { m_message = message; }

 private:
  ERROR_TYPE m_errorType;
  std::string m_exceptionName;
  std::string m_message;
  bool m_isRetryable;
};

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_CLIENT_CLIENTERROR_H_