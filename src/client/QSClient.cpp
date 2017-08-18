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

#include "client/QSClient.h"

#include <assert.h>
#include <stdint.h>  // for uint64_t

#include <chrono>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <utility>

#include "qingstor-sdk-cpp/Bucket.h"
#include "qingstor-sdk-cpp/HttpCommon.h"
#include "qingstor-sdk-cpp/QingStor.h"
#include "qingstor-sdk-cpp/QsConfig.h"

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/TimeUtils.h"
#include "base/Utils.h"
#include "client/ClientConfiguration.h"
#include "client/ClientImpl.h"
#include "client/Constants.h"
#include "client/Protocol.h"
#include "client/QSClientImpl.h"
#include "client/QSClientUtils.h"
#include "client/QSError.h"
#include "client/URI.h"
#include "data/Cache.h"
#include "data/Directory.h"
#include "data/FileMetaData.h"
#include "filesystem/Drive.h"

namespace QS {

namespace Client {

using QingStor::Bucket;
using QingStor::HeadObjectInput;
using QingStor::Http::HttpResponseCode;
using QingStor::ListObjectsInput;
using QingStor::PutObjectInput;
using QingStor::QingStorService;
using QingStor::QsConfig;  // sdk config

using QS::FileSystem::Drive;
using QS::StringUtils::LTrim;
using QS::TimeUtils::SecondsToRFC822GMT;
using QS::Utils::AppendPathDelim;
using std::chrono::milliseconds;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

namespace {

string BuildXQSSourceString(const string &objKey) {
  const auto &clientConfig = ClientConfiguration::Instance();
  // format: /bucket-name/object-key
  string ret = "/";
  ret.append(clientConfig.GetBucket());
  ret.append("/");
  ret.append(LTrim(objKey, '/'));
  return ret;
}

}  // namespace

static std::once_flag onceFlagGetClientImpl;
static std::once_flag onceFlagStartService;
unique_ptr<QingStor::QingStorService> QSClient::m_qingStorService = nullptr;

// TODO(jim): add template for retrying code

// --------------------------------------------------------------------------
QSClient::QSClient() : Client() {
  StartQSService();
  InitializeClientImpl();
}

// --------------------------------------------------------------------------
QSClient::~QSClient() { CloseQSService(); }

// --------------------------------------------------------------------------
bool QSClient::Connect() {
  auto outcome = GetQSClientImpl()->HeadBucket();
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadBucket();
    ++attemptedRetries;
  }

  /*   auto err =
        ListDirectory("/");  // TODO(jim): for test only, remove */
  if (outcome.IsSuccess()) {
    // Update root node of the tree
    auto &dirTree = Drive::Instance().GetDirectoryTree();
    if (dirTree) {
      dirTree->Grow(QS::Data::BuildDefaultDirectoryMeta("/"));
    }

    // Build up the root level of directory tree asynchornizely.
    auto receivedHandler = [](const ClientError<QSError> &err) {
      DebugErrorIf(!IsGoodQSError(err), GetMessageForQSError(err));
    };
    GetExecutor()->SubmitAsync(receivedHandler,
                               [this] { return ListDirectory("/"); });
    return true;
  } else {
    DebugError(GetMessageForQSError(outcome.GetError()));
    return false;
  }
}

// --------------------------------------------------------------------------
bool QSClient::DisConnect() {
  // TODO(jim):
  return true;
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteFile(const string &filePath) {
  // TODO(jim):
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DeleteDirectory(const string &dirPath) {
  // TODO(jim):
  // call list object
  // and recursive delete all object under
  // can we use delete multiple objects
  // maybe can call deleteMultipleObjects
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MakeFile(const string &filePath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::MakeDirectory(const string &dirPath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::RenameFile(const string &filePath,
                                          const string &newFilePath) {
  // TODO(jim):
  PutObjectInput input;
  input.SetXQSMoveSource(BuildXQSSourceString(filePath));

  auto outcome = GetQSClientImpl()->PutObject(newFilePath, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->PutObject(newFilePath, &input);
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto &drive = Drive::Instance();
    auto &dirTree = drive.GetDirectoryTree();
    if (dirTree) {
      dirTree->Rename(filePath, newFilePath);
    }
    auto &cache = drive.GetCache();
    if (cache) {
      cache->Rename(filePath, newFilePath);
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::RenameDirectory(const string &dirPath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DownloadFile(const string &filePath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::DownloadDirectory(const string &dirPath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::UploadFile(const string &filePath) {
  
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::UploadDirectory(const string &dirPath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::ReadFile(const string &filePath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::ListDirectory(const string &dirPath) {
  ListObjectsInput listObjInput;
  listObjInput.SetLimit(Constants::BucketListObjectsLimit);
  listObjInput.SetDelimiter(QS::Utils::GetPathDelimiter());
  if (!QS::Utils::IsRootDirectory(dirPath)) {
    string prefix = AppendPathDelim(LTrim(dirPath, '/'));
    listObjInput.SetPrefix(prefix);
  }
  bool resultTruncated = false;
  // Set maxCount for a single list operation.
  // This will request for ListObjects seperately, so we can construct
  // directory tree gradually. This will be helpful for the performance
  // if there are a huge number of objects to list.
  uint64_t maxCount = static_cast<uint64_t>(Constants::BucketListObjectsLimit);
  do {
    auto outcome = GetQSClientImpl()->ListObjects(&listObjInput,
                                                  &resultTruncated, maxCount);
    unsigned attemptedRetries = 0;
    while (
        !outcome.IsSuccess() &&
        GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
      int32_t sleepMilliseconds =
          GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                           attemptedRetries);
      RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
      outcome = GetQSClientImpl()->ListObjects(&listObjInput, &resultTruncated,
                                               maxCount);
      ++attemptedRetries;
    }

    if (outcome.IsSuccess()) {
      for (auto &listObjOutput : outcome.GetResult()) {
        auto fileMetaDatas =
            QSClientUtils::ListObjectsOutputToFileMetaDatas(listObjOutput);
        auto &drive = QS::FileSystem::Drive::Instance();
        auto &dirTree = drive.GetDirectoryTree();
        if (!dirTree) break;
        // DO NOT invoking update directory rescursively
        auto res = drive.GetNode(dirPath, false);
        auto dirNode = res.first.lock();
        bool modified = res.second;
        if (dirNode && modified) {
          dirTree->UpdateDiretory(dirPath, std::move(fileMetaDatas));
        } else {  // directory not existing at this moment
          // Add its children to dir tree
          dirTree->Grow(std::move(fileMetaDatas));
        }
      }
    } else {
      return outcome.GetError();
    }
  } while (resultTruncated);
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::WriteFile(const string &filePath) {
  // PutObject
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::WriteDirectory(const string &dirPath) {
  return ClientError<QSError>(QSError::GOOD, false);
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Stat(const string &path, time_t modifiedSince,
                                    bool *modified) {
  if (modified != nullptr) {
    *modified = false;
  }
  HeadObjectInput input;
  if (modifiedSince > 0) {
    input.SetIfModifiedSince(SecondsToRFC822GMT(modifiedSince));
  }

  auto outcome = GetQSClientImpl()->HeadObject(path, &input);
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->HeadObject(path, &input);
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    if (res.GetResponseCode() != HttpResponseCode::NOT_MODIFIED) {
      if (modified != nullptr) {
        *modified = true;
      }
      auto fileMetaData =
          QSClientUtils::HeadObjectOutputToFileMetaData(path, res);
      auto &dirTree = Drive::Instance().GetDirectoryTree();
      if (dirTree) {
        dirTree->Grow(std::move(fileMetaData));  // add Node to dir tree
      }
    }
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
ClientError<QSError> QSClient::Statvfs(struct statvfs *stvfs) {
  assert(stvfs != nullptr);
  if (stvfs == nullptr) {
    DebugError("Null statvfs parameter");
    return ClientError<QSError>(QSError::PARAMETER_MISSING, false);
  }

  auto outcome = GetQSClientImpl()->GetBucketStatistics();
  unsigned attemptedRetries = 0;
  while (!outcome.IsSuccess() &&
         GetRetryStrategy().ShouldRetry(outcome.GetError(), attemptedRetries)) {
    int32_t sleepMilliseconds =
        GetRetryStrategy().CalculateDelayBeforeNextRetry(outcome.GetError(),
                                                         attemptedRetries);
    RetryRequestSleep(std::chrono::milliseconds(sleepMilliseconds));
    outcome = GetQSClientImpl()->GetBucketStatistics();
    ++attemptedRetries;
  }

  if (outcome.IsSuccess()) {
    auto res = outcome.GetResult();
    QSClientUtils::GetBucketStatisticsOutputToStatvfs(res, stvfs);
    return ClientError<QSError>(QSError::GOOD, false);
  } else {
    return outcome.GetError();
  }
}

// --------------------------------------------------------------------------
const unique_ptr<QingStorService> &QSClient::GetQingStorService() {
  StartQSService();
  return m_qingStorService;
}

// --------------------------------------------------------------------------
const shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() const {
  return const_cast<QSClient *>(this)->GetQSClientImpl();
}

// --------------------------------------------------------------------------
shared_ptr<QSClientImpl> &QSClient::GetQSClientImpl() {
  std::call_once(onceFlagGetClientImpl, [this] {
    auto r = GetClientImpl();
    assert(r);
    FatalIf(!r, "QSClient is initialized with null QSClientImpl");
    auto p = dynamic_cast<QSClientImpl *>(r.get());
    m_qsClientImpl = std::move(shared_ptr<QSClientImpl>(r, p));
  });
  return m_qsClientImpl;
}

// --------------------------------------------------------------------------
void QSClient::StartQSService() {
  std::call_once(onceFlagStartService, [] {
    const auto &clientConfig = ClientConfiguration::Instance();
    // Must set sdk log at beginning, otherwise sdk will broken due to
    // uninitialization of plog sdk depended on.
    QingStorService::initService(clientConfig.GetClientLogFile());

    static QsConfig sdkConfig(clientConfig.GetAccessKeyId(),
                              clientConfig.GetSecretKey());
    sdkConfig.m_LogLevel =
        GetClientLogLevelName(clientConfig.GetClientLogLevel());
    sdkConfig.m_AdditionalUserAgent = clientConfig.GetAdditionalAgent();
    sdkConfig.m_Host = Http::HostToString(clientConfig.GetHost());
    sdkConfig.m_Protocol = Http::ProtocolToString(clientConfig.GetProtocol());
    sdkConfig.m_Port = clientConfig.GetPort();
    sdkConfig.m_ConnectionRetries = clientConfig.GetConnectionRetries();
    m_qingStorService =
        unique_ptr<QingStorService>(new QingStorService(sdkConfig));
  });
}

// --------------------------------------------------------------------------
void QSClient::CloseQSService() { QingStorService::shutdownService(); }

// --------------------------------------------------------------------------
void QSClient::InitializeClientImpl() {
  const auto &clientConfig = ClientConfiguration::Instance();
  if (GetQSClientImpl()->GetBucket()) return;
  GetQSClientImpl()->SetBucket(unique_ptr<Bucket>(
      new Bucket(m_qingStorService->GetConfig(), clientConfig.GetBucket(),
                 clientConfig.GetZone())));
}

}  // namespace Client
}  // namespace QS
