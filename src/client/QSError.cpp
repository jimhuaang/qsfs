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

#include "base/HashUtils.h"

namespace QS {

namespace Client {

using QingStor::Http::HttpResponseCode;  // sdk http response code
using std::string;
using std::to_string;
using std::unordered_map;

// --------------------------------------------------------------------------
QSError StringToQSError(const string &errorCode) {
  static unordered_map<string, QSError, QS::HashUtils::StringHash>
      errorCodeToTypeMap = {
          {"Unknow",                      QSError::UNKNOWN},
          {"Good",                        QSError::GOOD},
          {"AccessDenied",                QSError::ACCESS_DENIED},
          {"AccessKeyIdInvalid",          QSError::ACCESS_KEY_ID_INVALIDE},
          {"ActionInvalid",               QSError::ACTION_INVALID},
          {"ActionMissing",               QSError::ACTION_MISSING},
          {"AuthenticationTokenMissing",  QSError::AUTHENTICATION_TOKEN_MISSING},
          {"BucketAlreadyOwnedByYou",     QSError::BUCKET_ALREADY_OWNED_BY_YOU},
          {"BucketNotExist",              QSError::BUCKET_NOT_EXIST},
          {"ClientUnrecognied",           QSError::CLIENT_UNRECOGNIZED},
          {"ClientTokenIdInvalid",        QSError::CLIENT_TOKEN_ID_INVALID},
          {"InternalFailure",             QSError::INTERNAL_FAILURE},
          {"KeyNotExist",                 QSError::KEY_NOT_EXIST},
          {"NetworkConnection",           QSError::NETWORK_CONNECTION},
          {"NoSuchGetObject",             QSError::NO_SUCH_GET_OBJECT},
          {"NoSuchHeadObject",            QSError::NO_SUCH_HEAD_OBJECT},
          {"NoSuchListMultipart",         QSError::NO_SUCH_LIST_MULTIPART},
          {"NoSuchListMultipartUploads",  QSError::NO_SUCH_LIST_MULTIPART_UPLOADS},
          {"NoSuchListObjects",           QSError::NO_SUCH_LIST_OBJECTS},
          {"NoSuchPutObject",             QSError::NO_SUCH_PUT_OBJECT},
          {"NoSuchUpload",                QSError::NO_SUCH_UPLOAD,},
          {"ObjectAlreadyInActiveTier",   QSError::OBJECT_ALREADY_IN_ACTIVE_TIER},
          {"ObjectNotInActiveTier",       QSError::OBJECT_NOT_IN_ACTIVE_TIER},
          {"ParameterCombinationInvalid", QSError::PARAMETER_COMBINATION_INVALID},
          {"ParameterMissing",            QSError::PARAMETER_MISSING},
          {"ParameterValueInvalid",       QSError::PARAMETER_VALUE_INAVLID},
          {"QueryParameterInvalid",       QSError::QUERY_PARAMETER_INVALID},
          {"RequestExpired",              QSError::REQUEST_EXPIRED},
          {"ResourceNotFound",            QSError::RESOURCE_NOT_FOUND},
          {"ServiceUnavailable",          QSError::SERVICE_UNAVAILABLE},
          {"SignatureDoesNotMatch",       QSError::SIGNATURE_DOES_NOT_MATCH},
          {"SignatureIncompleted",        QSError::SIGNATURE_INCOMPLETED},
          {"SignatureInvalid",            QSError::SIGNATURE_INVALID},
          {"SDKConfigureFileInvalid",     QSError::SDK_CONFIGURE_FILE_INAVLID},
          {"SDKRequestSendError",         QSError::SDK_REQUEST_SEND_ERR},
          // Add other errors here.
      };
  auto it = errorCodeToTypeMap.find(errorCode);
  return it != errorCodeToTypeMap.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
std::string QSErrorToString(QSError err){
  static unordered_map<QSError, string, QS::HashUtils::EnumHash>
      errorTypeToCodeMap = {
          {QSError::UNKNOWN                       , "Unknow"                      },
          {QSError::GOOD                          , "Good"                        },
          {QSError::ACCESS_DENIED                 , "AccessDenied"                },
          {QSError::ACCESS_KEY_ID_INVALIDE        , "AccessKeyIdInvalid"          },
          {QSError::ACTION_INVALID                , "ActionInvalid"               },
          {QSError::ACTION_MISSING                , "ActionMissing"               },
          {QSError::AUTHENTICATION_TOKEN_MISSING  , "AuthenticationTokenMissing"  },
          {QSError::BUCKET_ALREADY_OWNED_BY_YOU   , "BucketAlreadyOwnedByYou"     },
          {QSError::BUCKET_NOT_EXIST              , "BucketNotExist"              },
          {QSError::CLIENT_UNRECOGNIZED           , "ClientUnrecognied"           },
          {QSError::CLIENT_TOKEN_ID_INVALID       , "ClientTokenIdInvalid"        },
          {QSError::INTERNAL_FAILURE              , "InternalFailure"             },
          {QSError::KEY_NOT_EXIST                 , "KeyNotExist"                 },
          {QSError::NETWORK_CONNECTION            , "NetworkConnection"           },
          {QSError::NO_SUCH_GET_OBJECT            , "NoSuchGetObject"             },
          {QSError::NO_SUCH_HEAD_OBJECT           , "NoSuchHeadObject"            },
          {QSError::NO_SUCH_LIST_MULTIPART        , "NoSuchListMultipart"         },
          {QSError::NO_SUCH_LIST_MULTIPART_UPLOADS, "NoSuchListMultipartUploads"  },
          {QSError::NO_SUCH_LIST_OBJECTS          , "NoSuchListObjects"           },
          {QSError::NO_SUCH_PUT_OBJECT            , "NoSuchPutObject"             },
          {QSError::NO_SUCH_UPLOAD                , "NoSuchUpload"                },
          {QSError::OBJECT_ALREADY_IN_ACTIVE_TIER , "ObjectAlreadyInActiveTier"   },
          {QSError::OBJECT_NOT_IN_ACTIVE_TIER     , "ObjectNotInActiveTier"       },
          {QSError::PARAMETER_COMBINATION_INVALID , "ParameterCombinationInvalid" },
          {QSError::PARAMETER_MISSING             , "ParameterMissing"            },
          {QSError::PARAMETER_VALUE_INAVLID       , "ParameterValueInvalid"       },
          {QSError::QUERY_PARAMETER_INVALID       , "QueryParameterInvalid"       },
          {QSError::REQUEST_EXPIRED               , "RequestExpired"              },
          {QSError::RESOURCE_NOT_FOUND            , "ResourceNotFound"            },
          {QSError::SERVICE_UNAVAILABLE           , "ServiceUnavailable"          },
          {QSError::SIGNATURE_DOES_NOT_MATCH      , "SignatureDoesNotMatch"       },
          {QSError::SIGNATURE_INCOMPLETED         , "SignatureIncompleted"        },
          {QSError::SIGNATURE_INVALID             , "SignatureInvalid"            },
          {QSError::SDK_CONFIGURE_FILE_INAVLID    , "SDKConfigureFileInvalid"     },
          {QSError::SDK_REQUEST_SEND_ERR          , "SDKRequestSendError"         },
          // Add other errors here.
      };
  auto it = errorTypeToCodeMap.find(err);
  return it != errorTypeToCodeMap.end() ? it->second : "Unknow";
}

// --------------------------------------------------------------------------
ClientError<QSError> GetQSErrorForCode(const string &errorCode) {
  return ClientError<QSError>(StringToQSError(errorCode),false);
}

// --------------------------------------------------------------------------
std::string GetMessageForQSError(const ClientError<QSError> &error) {
  return QSErrorToString(error.GetError()) + ", " + error.GetExceptionName() +
         ":" + error.GetMessage();
}

// --------------------------------------------------------------------------
bool IsGoodQSError(const ClientError<QSError> &error){
  return error.GetError() == QSError::GOOD;
}

// --------------------------------------------------------------------------
QSError SDKErrorToQSError(QsError sdkErr) {
  static unordered_map<QsError, QSError, QS::HashUtils::EnumHash> sdkErrToQSErrMap =
      {
          {QsError::QS_ERR_NO_ERROR,              QSError::GOOD},
          {QsError::QS_ERR_INVAILD_CONFIG_FILE,   QSError::SDK_CONFIGURE_FILE_INAVLID},
          {QsError::QS_ERR_NO_REQUIRED_PARAMETER, QSError::PARAMETER_MISSING},
          {QsError::QS_ERR_SING_ERROR,            QSError::SIGNATURE_INVALID},
          {QsError::QS_ERR_SEND_REQUEST_ERROR,    QSError::SDK_REQUEST_SEND_ERR},
          // Add others here
      };
  auto it = sdkErrToQSErrMap.find(sdkErr);
  return it != sdkErrToQSErrMap.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
bool SDKResponseSuccess(QsError sdkErr) {
  return sdkErr == QsError::QS_ERR_NO_ERROR;
}

// --------------------------------------------------------------------------
string SDKResponseCodeToName(HttpResponseCode code) {
  static unordered_map<HttpResponseCode, string, QS::HashUtils::EnumHash>
      sdkResponseCodeToNameMap = {
          {HttpResponseCode::REQUEST_NOT_MADE,                "RequestNotMade"},                 // 0
          {HttpResponseCode::CONTINUE,                        "Continue"},                       // 100
          {HttpResponseCode::SWITCHING_PROTOCOLS,             "SwitchingProtocols"},             // 101
          {HttpResponseCode::PROCESSING,                      "Processing"},                     // 102
          {HttpResponseCode::OK,                              "Ok"},                             // 200
          {HttpResponseCode::CREATED,                         "Created"},                        // 201
          {HttpResponseCode::ACCEPTED,                        "Accepted"},                       // 202
          {HttpResponseCode::NON_AUTHORITATIVE_INFORMATION,   "NonAuthoritativeInformation"},    // 203
          {HttpResponseCode::NO_CONTENT,                      "NoContent"},                      // 204
          {HttpResponseCode::RESET_CONTENT,                   "ResetContent"},                   // 205
          {HttpResponseCode::PARTIAL_CONTENT,                 "PartialContent"},                 // 206
          {HttpResponseCode::MULTI_STATUS,                    "MultiStatus"},                    // 207
          {HttpResponseCode::ALREADY_REPORTED,                "AlreadyReported"},                // 208
          {HttpResponseCode::IM_USED,                         "IMUsed"},                         // 226
          {HttpResponseCode::MULTIPLE_CHOICES,                "MultipleChoices"},                // 300
          {HttpResponseCode::MOVED_PERMANENTLY,               "MovedPermanently"},               // 301
          {HttpResponseCode::FOUND,                           "Found"},                          // 302
          {HttpResponseCode::SEE_OTHER,                       "SeeOther"},                       // 303
          {HttpResponseCode::NOT_MODIFIED,                    "NotModified"},                    // 304
          {HttpResponseCode::USE_PROXY,                       "UseProxy"},                       // 305
          {HttpResponseCode::SWITCH_PROXY,                    "SwitchProxy"},                    // 306
          {HttpResponseCode::TEMPORARY_REDIRECT,              "TemporaryRedirect"},              // 307
          {HttpResponseCode::PERMANENT_REDIRECT,              "PermanentRedirect"},              // 308
          {HttpResponseCode::BAD_REQUEST,                     "BadRequest"},                     // 400
          {HttpResponseCode::UNAUTHORIZED,                    "Unauthorized"},                   // 401
          {HttpResponseCode::PAYMENT_REQUIRED,                "PaymentRequired"},                // 402
          {HttpResponseCode::FORBIDDEN,                       "Forbidden"},                      // 403
          {HttpResponseCode::NOT_FOUND,                       "NotFound"},                       // 404
          {HttpResponseCode::METHOD_NOT_ALLOWED,              "MethodNotAllowed"},               // 405
          {HttpResponseCode::NOT_ACCEPTABLE,                  "NotAcceptable"},                  // 406
          {HttpResponseCode::PROXY_AUTHENTICATION_REQUIRED,   "ProxyAuthenticationRequired"},    // 407
          {HttpResponseCode::REQUEST_TIMEOUT,                 "RequestTimeOut"},                 // 408
          {HttpResponseCode::CONFLICT,                        "Conflict"},                       // 409
          {HttpResponseCode::GONE,                            "Gone"},                           // 410
          {HttpResponseCode::LENGTH_REQUIRED,                 "LengthRequired"},                 // 411
          {HttpResponseCode::PRECONDITION_FAILED,             "PerconditionFailed"},             // 412
          {HttpResponseCode::REQUEST_ENTITY_TOO_LARGE,        "RequestEntityTooLarge"},          // 413
          {HttpResponseCode::REQUEST_URI_TOO_LONG,            "RequestURITooLong"},              // 414
          {HttpResponseCode::UNSUPPORTED_MEDIA_TYPE,          "UnsupportedMediaType"},           // 415
          {HttpResponseCode::REQUESTED_RANGE_NOT_SATISFIABLE, "RequestedRangeNotSatisfiable"},   // 416
          {HttpResponseCode::EXPECTATION_FAILED,              "ExpectationFailed"},              // 417
          {HttpResponseCode::IM_A_TEAPOT,                     "ImATeapot"},                      // 418
          {HttpResponseCode::AUTHENTICATION_TIMEOUT,          "AuthenticationTimeout"},          // 419
          {HttpResponseCode::METHOD_FAILURE,                  "MethodFailure"},                  // 420
          {HttpResponseCode::UNPROC_ENTITY,                   "UnprocEntity"},                   // 422
          {HttpResponseCode::LOCKED,                          "Locked"},                         // 423
          {HttpResponseCode::FAILED_DEPENDENCY,               "FailedDependency"},               // 424
          {HttpResponseCode::UPGRADE_REQUIRED,                "UpgradeRequired"},                // 426
          {HttpResponseCode::PRECONDITION_REQUIRED,           "PreconditionRequired"},           // 427
          {HttpResponseCode::TOO_MANY_REQUESTS,               "TooManyRequests"},                // 429
          {HttpResponseCode::REQUEST_HEADER_FIELDS_TOO_LARGE, "RequestHeaderFieldsTooLarge"},    // 431
          {HttpResponseCode::LOGIN_TIMEOUT,                   "LoginTimeout"},                   // 440
          {HttpResponseCode::NO_RESPONSE,                     "NoResponse"},                     // 444
          {HttpResponseCode::RETRY_WITH,                      "RetryWith"},                      // 449
          {HttpResponseCode::BLOCKED,                         "Blocked"},                        // 450
          {HttpResponseCode::REDIRECT,                        "Redirect"},                       // 451
          {HttpResponseCode::REQUEST_HEADER_TOO_LARGE,        "RequestHeaderTooLarge"},          // 494
          {HttpResponseCode::CERT_ERROR,                      "CretError"},                      // 495
          {HttpResponseCode::NO_CERT,                         "NoCret"},                         // 496
          {HttpResponseCode::HTTP_TO_HTTPS,                   "HttpToHttps"},                    // 497
          {HttpResponseCode::CLIENT_CLOSED_TO_REQUEST,        "ClientClosedToRequest"},          // 499
          {HttpResponseCode::INTERNAL_SERVER_ERROR,           "InternalServerError"},            // 500
          {HttpResponseCode::NOT_IMPLEMENTED,                 "NotImplemented"},                 // 501
          {HttpResponseCode::BAD_GATEWAY,                     "BadGateway"},                     // 502
          {HttpResponseCode::SERVICE_UNAVAILABLE,             "ServiceUnavailable"},             // 503
          {HttpResponseCode::GATEWAY_TIMEOUT,                 "GatewayTimeout"},                 // 504
          {HttpResponseCode::HTTP_VERSION_NOT_SUPPORTED,      "HttpVersionNotSupported"},        // 505
          {HttpResponseCode::VARIANT_ALSO_NEGOTIATES,         "VariantAlsoNegotiates"},          // 506
          {HttpResponseCode::INSUFFICIENT_STORAGE,            "InsufficientStorage"},            // 506
          {HttpResponseCode::LOOP_DETECTED,                   "LoopDetected"},                   // 508
          {HttpResponseCode::BANDWIDTH_LIMIT_EXCEEDED,        "BandwithLimitExceeded"},          // 509
          {HttpResponseCode::NOT_EXTENDED,                    "NotExtended"},                    // 510
          {HttpResponseCode::NETWORK_AUTHENTICATION_REQUIRED, "NetworkAuthenticationRequired"},  // 511
          {HttpResponseCode::NETWORK_READ_TIMEOUT,            "NetworkReadTimeout"},             // 598
          {HttpResponseCode::NETWORK_CONNECT_TIMEOUT,         "NetworkConnectTimeout"},          // 599
      };
  auto it = sdkResponseCodeToNameMap.find(code);
  return it != sdkResponseCodeToNameMap.end() ? it->second
                                              : "UnknownQingStorResponseCode";
}

// --------------------------------------------------------------------------
int SDKResponseCodeToInt(HttpResponseCode code) {
  static unordered_map<HttpResponseCode, int, QS::HashUtils::EnumHash>
      sdkResponseCodeToNumMap = {
          {HttpResponseCode::REQUEST_NOT_MADE,                 0},
          {HttpResponseCode::CONTINUE,                         100},
          {HttpResponseCode::SWITCHING_PROTOCOLS,              101},
          {HttpResponseCode::PROCESSING,                       102},
          {HttpResponseCode::OK,                               200},
          {HttpResponseCode::CREATED,                          201},
          {HttpResponseCode::ACCEPTED,                         202},
          {HttpResponseCode::NON_AUTHORITATIVE_INFORMATION,    203},
          {HttpResponseCode::NO_CONTENT,                       204},
          {HttpResponseCode::RESET_CONTENT,                    205},
          {HttpResponseCode::PARTIAL_CONTENT,                  206},
          {HttpResponseCode::MULTI_STATUS,                     207},
          {HttpResponseCode::ALREADY_REPORTED,                 208},
          {HttpResponseCode::IM_USED,                          226},
          {HttpResponseCode::MULTIPLE_CHOICES,                 300},
          {HttpResponseCode::MOVED_PERMANENTLY,                301},
          {HttpResponseCode::FOUND,                            302},
          {HttpResponseCode::SEE_OTHER,                        303},
          {HttpResponseCode::NOT_MODIFIED,                     304},
          {HttpResponseCode::USE_PROXY,                        305},
          {HttpResponseCode::SWITCH_PROXY,                     306},
          {HttpResponseCode::TEMPORARY_REDIRECT,               307},
          {HttpResponseCode::PERMANENT_REDIRECT,               308},
          {HttpResponseCode::BAD_REQUEST,                      400},
          {HttpResponseCode::UNAUTHORIZED,                     401},
          {HttpResponseCode::PAYMENT_REQUIRED,                 402},
          {HttpResponseCode::FORBIDDEN,                        403},
          {HttpResponseCode::NOT_FOUND,                        404},
          {HttpResponseCode::METHOD_NOT_ALLOWED,               405},
          {HttpResponseCode::NOT_ACCEPTABLE,                   406},
          {HttpResponseCode::PROXY_AUTHENTICATION_REQUIRED,    407},
          {HttpResponseCode::REQUEST_TIMEOUT,                  408},
          {HttpResponseCode::CONFLICT,                         409},
          {HttpResponseCode::GONE,                             410},
          {HttpResponseCode::LENGTH_REQUIRED,                  411},
          {HttpResponseCode::PRECONDITION_FAILED,              412},
          {HttpResponseCode::REQUEST_ENTITY_TOO_LARGE,         413},
          {HttpResponseCode::REQUEST_URI_TOO_LONG,             414},
          {HttpResponseCode::UNSUPPORTED_MEDIA_TYPE,           415},
          {HttpResponseCode::REQUESTED_RANGE_NOT_SATISFIABLE,  416},
          {HttpResponseCode::EXPECTATION_FAILED,               417},
          {HttpResponseCode::IM_A_TEAPOT,                      418},
          {HttpResponseCode::AUTHENTICATION_TIMEOUT,           419},
          {HttpResponseCode::METHOD_FAILURE,                   420},
          {HttpResponseCode::UNPROC_ENTITY,                    422},
          {HttpResponseCode::LOCKED,                           423},
          {HttpResponseCode::FAILED_DEPENDENCY,                424},
          {HttpResponseCode::UPGRADE_REQUIRED,                 426},
          {HttpResponseCode::PRECONDITION_REQUIRED,            427},
          {HttpResponseCode::TOO_MANY_REQUESTS,                429},
          {HttpResponseCode::REQUEST_HEADER_FIELDS_TOO_LARGE,  431},
          {HttpResponseCode::LOGIN_TIMEOUT,                    440},
          {HttpResponseCode::NO_RESPONSE,                      444},
          {HttpResponseCode::RETRY_WITH,                       449},
          {HttpResponseCode::BLOCKED,                          450},
          {HttpResponseCode::REDIRECT,                         451},
          {HttpResponseCode::REQUEST_HEADER_TOO_LARGE,         494},
          {HttpResponseCode::CERT_ERROR,                       495},
          {HttpResponseCode::NO_CERT,                          496},
          {HttpResponseCode::HTTP_TO_HTTPS,                    497},
          {HttpResponseCode::CLIENT_CLOSED_TO_REQUEST,         499},
          {HttpResponseCode::INTERNAL_SERVER_ERROR,            500},
          {HttpResponseCode::NOT_IMPLEMENTED,                  501},
          {HttpResponseCode::BAD_GATEWAY,                      502},
          {HttpResponseCode::SERVICE_UNAVAILABLE,              503},
          {HttpResponseCode::GATEWAY_TIMEOUT,                  504},
          {HttpResponseCode::HTTP_VERSION_NOT_SUPPORTED,       505},
          {HttpResponseCode::VARIANT_ALSO_NEGOTIATES,          506},
          {HttpResponseCode::INSUFFICIENT_STORAGE,             506},
          {HttpResponseCode::LOOP_DETECTED,                    508},
          {HttpResponseCode::BANDWIDTH_LIMIT_EXCEEDED,         509},
          {HttpResponseCode::NOT_EXTENDED,                     510},
          {HttpResponseCode::NETWORK_AUTHENTICATION_REQUIRED,  511},
          {HttpResponseCode::NETWORK_READ_TIMEOUT,             598},
          {HttpResponseCode::NETWORK_CONNECT_TIMEOUT,          599},
      };
  auto it = sdkResponseCodeToNumMap.find(code);
  return it != sdkResponseCodeToNumMap.end() ? it->second : -1;
}

// --------------------------------------------------------------------------
string SDKResponseCodeToString(HttpResponseCode code) {
  return SDKResponseCodeToName(code) + "(" + to_string(SDKResponseCodeToInt(code)) + ")";
}

}  // namespace Client
}  // namespace QS
