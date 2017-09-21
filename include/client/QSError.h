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

#include "qingstor-sdk-cpp/HttpCommon.h"
#include "qingstor-sdk-cpp/QsErrors.h"

#include "client/ClientError.h"

namespace QS {

namespace Client {

enum class QSError {
  UNKNOWN,
  GOOD,
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
  NO_SUCH_LIST_MULTIPART,
  NO_SUCH_LIST_MULTIPART_UPLOADS,
  NO_SUCH_LIST_OBJECTS,
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
  SDK_CONFIGURE_FILE_INAVLID,
  SDK_REQUEST_SEND_ERR
};

QSError StringToQSError(const std::string &errorCode);
std::string QSErrorToString(QSError err);

ClientError<QSError> GetQSErrorForCode(const std::string &errorCode);
std::string GetMessageForQSError(const ClientError<QSError> &error);
bool IsGoodQSError(const ClientError<QSError> &error);

QSError SDKErrorToQSError(QsError sdkErr);
QSError SDKResponseToQSError(QingStor::Http::HttpResponseCode code);
bool SDKShouldRetry(QingStor::Http::HttpResponseCode code);
bool SDKResponseSuccess(QsError sdkErr, QingStor::Http::HttpResponseCode code);

std::string SDKResponseCodeToName(QingStor::Http::HttpResponseCode code);
int SDKResponseCodeToInt(QingStor::Http::HttpResponseCode code);
std::string SDKResponseCodeToString(QingStor::Http::HttpResponseCode code);

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_CLIENT_QSERROR_H_
