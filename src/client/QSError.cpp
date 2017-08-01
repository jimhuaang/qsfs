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

using QingStor::Http::HttpResponseCode;  // sdk http response code
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
          {"ParameterCombinationInvalid",
           QSError::PARAMETER_COMBINATION_INVALID},
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
          {"SDKConfigureFileInvalid", QSError::SDK_CONFIGURE_FILE_INAVLID},
          {"SDKRequestSendError", QSError::SDK_REQUEST_SEND_ERR},
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

// --------------------------------------------------------------------------
QSError SDKErrorToQSError(const QsError &sdkErr) {
  static unordered_map<QsError, QSError, QS::Utils::EnumHash> sdkErrToQSErrMap =
      {
          {QsError::QS_ERR_NO_ERROR, QSError::UNKNOWN},
          {QsError::QS_ERR_INVAILD_CONFIG_FILE,
           QSError::SDK_CONFIGURE_FILE_INAVLID},
          {QsError::QS_ERR_NO_REQUIRED_PARAMETER, QSError::PARAMETER_MISSING},
          {QsError::QS_ERR_SING_ERROR, QSError::SIGNATURE_INVALID},
          {QsError::QS_ERR_SEND_REQUEST_ERROR, QSError::SDK_REQUEST_SEND_ERR},
          // Add others here
      };
  auto it = sdkErrToQSErrMap.find(sdkErr);
  return it != sdkErrToQSErrMap.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
bool SDKRequestSuccess(const QsError &sdkErr) {
  return sdkErr == QsError::QS_ERR_NO_ERROR;
}

// --------------------------------------------------------------------------
string SDKResponseToString(const HttpResponseCode &code) {
  static unordered_map<HttpResponseCode, string, QS::Utils::EnumHash>
      sdkResponseCodeToNameMap = {
          {HttpResponseCode::REQUEST_NOT_MADE, "RequestNotMade"},                                // 0
          {HttpResponseCode::CONTINUE, "Continue"},                                              // 100
          {HttpResponseCode::SWITCHING_PROTOCOLS, "SwitchingProtocols"},                         // 101
          {HttpResponseCode::PROCESSING, "Processing"},                                          // 102
          {HttpResponseCode::OK, "Ok"},                                                          // 200
          {HttpResponseCode::CREATED, "Created"},                                                // 201
          {HttpResponseCode::ACCEPTED, "Accepted"},                                              // 202
          {HttpResponseCode::NON_AUTHORITATIVE_INFORMATION, "NonAuthoritativeInformation"},      // 203
          {HttpResponseCode::NO_CONTENT, "NoContent"},                                           // 204
          {HttpResponseCode::RESET_CONTENT, "ResetContent"},                                     // 205
          {HttpResponseCode::PARTIAL_CONTENT, "PartialContent"},                                 // 206
          {HttpResponseCode::MULTI_STATUS, "MultiStatus"},                                       // 207
          {HttpResponseCode::ALREADY_REPORTED, "AlreadyReported"},                               // 208
          {HttpResponseCode::IM_USED, "IMUsed"},                                                 // 226
          {HttpResponseCode::MULTIPLE_CHOICES, "MultipleChoices"},                               // 300
          {HttpResponseCode::MOVED_PERMANENTLY, "MovedPermanently"},                             // 301
          {HttpResponseCode::FOUND, "Found"},                                                    // 302
          {HttpResponseCode::SEE_OTHER, "SeeOther"},                                             // 303
          {HttpResponseCode::NOT_MODIFIED, "NotModified"},                                       // 304
          {HttpResponseCode::USE_PROXY, "UseProxy"},                                             // 305
          {HttpResponseCode::SWITCH_PROXY, "SwitchProxy"},                                       // 306
          {HttpResponseCode::TEMPORARY_REDIRECT, "TemporaryRedirect"},                           // 307
          {HttpResponseCode::PERMANENT_REDIRECT, "PermanentRedirect"},                           // 308
          {HttpResponseCode::BAD_REQUEST, "BadRequest"},                                         // 400
          {HttpResponseCode::UNAUTHORIZED, "Unauthorized"},                                      // 401
          {HttpResponseCode::PAYMENT_REQUIRED, "PaymentRequired"},                               // 402
          {HttpResponseCode::FORBIDDEN, "Forbidden"},                                            // 403
          {HttpResponseCode::NOT_FOUND, "NotFound"},                                             // 404
          {HttpResponseCode::METHOD_NOT_ALLOWED, "MethodNotAllowed"},                            // 405
          {HttpResponseCode::NOT_ACCEPTABLE, "NotAcceptable"},                                   // 406
          {HttpResponseCode::PROXY_AUTHENTICATION_REQUIRED, "ProxyAuthenticationRequired"},      // 407
          {HttpResponseCode::REQUEST_TIMEOUT, "RequestTimeOut"},                                 // 408
          {HttpResponseCode::CONFLICT, "Conflict"},                                              // 409
          {HttpResponseCode::GONE, "Gone"},                                                      // 410
          {HttpResponseCode::LENGTH_REQUIRED, "LengthRequired"},                                 // 411
          {HttpResponseCode::PRECONDITION_FAILED, "PerconditionFailed"},                         // 412
          {HttpResponseCode::REQUEST_ENTITY_TOO_LARGE, "RequestEntityTooLarge"},                 // 413
          {HttpResponseCode::REQUEST_URI_TOO_LONG, "RequestURITooLong"},                         // 414
          {HttpResponseCode::UNSUPPORTED_MEDIA_TYPE, "UnsupportedMediaType"},                    // 415
          {HttpResponseCode::REQUESTED_RANGE_NOT_SATISFIABLE, "RequestedRangeNotSatisfiable"},   // 416
          {HttpResponseCode::EXPECTATION_FAILED, "ExpectationFailed"},                           // 417
          {HttpResponseCode::IM_A_TEAPOT, "ImATeapot"},                                          // 418
          {HttpResponseCode::AUTHENTICATION_TIMEOUT, "AuthenticationTimeout"},                   // 419
          {HttpResponseCode::METHOD_FAILURE, "MethodFailure"},                                   // 420
          {HttpResponseCode::UNPROC_ENTITY, "UnprocEntity"},                                     // 422
          {HttpResponseCode::LOCKED, "Locked"},                                                  // 423
          {HttpResponseCode::FAILED_DEPENDENCY, "FailedDependency"},                             // 424
          {HttpResponseCode::UPGRADE_REQUIRED, "UpgradeRequired"},                               // 426
          {HttpResponseCode::PRECONDITION_REQUIRED, "PreconditionRequired"},                     // 427
          {HttpResponseCode::TOO_MANY_REQUESTS, "TooManyRequests"},                              // 429
          {HttpResponseCode::REQUEST_HEADER_FIELDS_TOO_LARGE, "RequestHeaderFieldsTooLarge"},    // 431
          {HttpResponseCode::LOGIN_TIMEOUT, "LoginTimeout"},                                     // 440
          {HttpResponseCode::NO_RESPONSE, "NoResponse"},                                         // 444
          {HttpResponseCode::RETRY_WITH, "RetryWith"},                                           // 449
          {HttpResponseCode::BLOCKED, "Blocked"},                                                // 450
          {HttpResponseCode::REDIRECT, "Redirect"},                                              // 451
          {HttpResponseCode::REQUEST_HEADER_TOO_LARGE, "RequestHeaderTooLarge"},                 // 494
          {HttpResponseCode::CERT_ERROR, "CretError"},                                           // 495
          {HttpResponseCode::NO_CERT, "NoCret"},                                                 // 496
          {HttpResponseCode::HTTP_TO_HTTPS, "HttpToHttps"},                                      // 497
          {HttpResponseCode::CLIENT_CLOSED_TO_REQUEST, "ClientClosedToRequest"},                 // 499
          {HttpResponseCode::INTERNAL_SERVER_ERROR, "InternalServerError"},                      // 500
          {HttpResponseCode::NOT_IMPLEMENTED, "NotImplemented"},                                 // 501
          {HttpResponseCode::BAD_GATEWAY, "BadGateway"},                                         // 502
          {HttpResponseCode::SERVICE_UNAVAILABLE, "ServiceUnavailable"},                         // 503
          {HttpResponseCode::GATEWAY_TIMEOUT, "GatewayTimeout"},                                 // 504
          {HttpResponseCode::HTTP_VERSION_NOT_SUPPORTED, "HttpVersionNotSupported"},             // 505
          {HttpResponseCode::VARIANT_ALSO_NEGOTIATES, "VariantAlsoNegotiates"},                  // 506
          {HttpResponseCode::INSUFFICIENT_STORAGE, "InsufficientStorage"},                       // 506
          {HttpResponseCode::LOOP_DETECTED, "LoopDetected"},                                     // 508
          {HttpResponseCode::BANDWIDTH_LIMIT_EXCEEDED, "BandwithLimitExceeded"},                 // 509
          {HttpResponseCode::NOT_EXTENDED, "NotExtended"},                                       // 510
          {HttpResponseCode::NETWORK_AUTHENTICATION_REQUIRED, "NetworkAuthenticationRequired"},  // 511
          {HttpResponseCode::NETWORK_READ_TIMEOUT, "NetworkReadTimeout"},                        // 598
          {HttpResponseCode::NETWORK_CONNECT_TIMEOUT, "NetworkConnectTimeout"},                  // 599
      };
  auto it = sdkResponseCodeToNameMap.find(code);
  return it != sdkResponseCodeToNameMap.end() ? it->second
                                              : "UnknownQingStorResponseCode";
}

}  // namespace Client
}  // namespace QS
