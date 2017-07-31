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

#include "client/QSError.h"

#include <unordered_map>

#include "base/Utils.h"

namespace QS {

namespace Client {

using std::string;
using std::unordered_map;

// --------------------------------------------------------------------------
ClientError<QSError> GetQSErrorForCode(const string &errorCode) {
  static unordered_map<string, QSError, QS::Utils::StringHash>
      errorCodeToTypeMap = {
          {"Unknow", QSError::UNKNOWN},
          {"AccessDenied", QSError::ACCESS_DENIED},
          {"AccessKeyIdInvalid", QSError::ACCESS_KEY_ID_INVALIDE},
          {"ActionInvalid", QSError::ACTION_INVALID},
          {"ActionMissing", QSError::ACTION_MISSING},
          {"AuthenticationTokenMissing", QSError::AUTHENTICATION_TOKEN_MISSING},
          {"BucketAlreadyOwnedByYou", QSError::BUCKET_ALREADY_OWNED_BY_YOU},
          {"BucketNotExist", QSError::BUCKET_NOT_EXIST},
          {"ClientUnrecognied", QSError::CLIENT_UNRECOGNIZED},
          {"ClientTokenIdInvalid", QSError::CLIENT_TOKEN_ID_INVALID},
          {"InternalFailure", QSError::INTERNAL_FAILURE},
          {"KeyNotExist", QSError::KEY_NOT_EXIST},
          {"NetworkConnection", QSError::NETWORK_CONNECTION},
          {"ObjectAlreadyInActiveTier", QSError::OBJECT_ALREADY_IN_ACTIVE_TIER},
          {"ObjectNotInActiveTier", QSError::OBJECT_NOT_IN_ACTIVE_TIER},
          {"ParameterCombinationInvalid", QSError::PARAMETER_COMBINATION_INVALID},
          {"ParameterMissing", QSError::PARAMETER_MISSING},
          {"ParameterValueInvalid", QSError::PARAMETER_VALUE_INAVLID},
          {"QueryParameterInvalid", QSError::QUERY_PARAMETER_INVALID},
          {"RequestExpired", QSError::REQUEST_EXPIRED},
          {"ResourceNotFound", QSError::RESOURCE_NOT_FOUND},
          {"ServiceUnavailable", QSError::SERVICE_UNAVAILABLE},
          {"SignatureDoesNotMatch", QSError::SIGNATURE_DOES_NOT_MATCH},
          {"SignatureIncompleted", QSError::SIGNATURE_INCOMPLETED},
          {"SignatureInvalid", QSError::SIGNATURE_INVALID},
          {"UploadNotExist", QSError::UPLOAD_NOT_EXIST},
          // TODO(Jim): Add other errors here.
      };
  auto it = errorCodeToTypeMap.find(errorCode);
  return it != errorCodeToTypeMap.end()
             ? ClientError<QSError>(it->second, false)
             : ClientError<QSError>(QSError::UNKNOWN, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> GetQSErrorForCode(const char *errorCode) {
  if (!errorCode) return ClientError<QSError>(QSError::UNKNOWN, false);
  return GetQSErrorForCode(string(errorCode));
}

}  // namespace Client
}  // namespace QS
