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

#ifndef _QSFS_FUSE_INCLUDE_CLIENT_QSERROR_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_CLIENT_QSERROR_H_  // NOLINT

#include <string>

#include "client/ClientError.h"

namespace QS {

namespace Client {

enum class QSError {
  UNKNOWN,
  ACCESS_DENIED,
  ACCESS_KEY_ID_INVALIDE,
  ACTION_INVALID,
  ACTION_MISSING,                // SDK should never allow
  AUTHENTICATION_TOKEN_MISSING,  // SDK should never allow
  BUCKET_ALREADY_OWNED_BY_YOU,
  BUCKET_NOT_EXIST,
  CLIENT_UNRECOGNIZED,
  CLIENT_TOKEN_ID_INVALID,
  INTERNAL_FAILURE,
  KEY_NOT_EXIST,
  NETWORK_CONNECTION,
  OBJECT_ALREADY_IN_ACTIVE_TIER,
  OBJECT_NOT_IN_ACTIVE_TIER,
  PARAMETER_COMBINATION_INVALID,
  PARAMETER_MISSING,  // SDK should never allow
  PARAMETER_VALUE_INAVLID,
  QUERY_PARAMETER_INVALID,
  REQUEST_EXPIRED,
  RESOURCE_NOT_FOUND,
  SERVICE_UNAVAILABLE,
  SIGNATURE_DOES_NOT_MATCH,
  SIGNATURE_INCOMPLETED,
  SIGNATURE_INVALID,
  UPLOAD_NOT_EXIST
};

ClientError<QSError> GetQSErrorForCode(const std::string &errorCode);
ClientError<QSError> GetQSErrorForCode(const char *errorCode);

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_CLIENT_QSERROR_H_