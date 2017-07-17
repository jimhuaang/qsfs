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

#include "base/Logging.h"

#include <assert.h>

#include <exception>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT

#include "glog/logging.h"

#include "base/Exception.h"
#include "base/LogLevel.h"
#include "base/Utils.h"
#include "qingstor/Configure.h"

namespace {

void InitializeGLog() {
  google::InitGoogleLogging(QS::QingStor::Configure::GetProgramName());
}

}  // namespace

namespace QS {

namespace Logging {

using QS::Exception::QSException;
using std::once_flag;
using std::unique_ptr;

static unique_ptr<Log> logInstance(nullptr);
static once_flag logOnceFlag;

void InitializeLogging(unique_ptr<Log> log) {
  assert(log);
  std::call_once(logOnceFlag, [&log] {
    logInstance = std::move(log);
    InitializeGLog();
  });
}

Log* GetLogInstance() {
  std::call_once(logOnceFlag, [] {
    logInstance = unique_ptr<Log>(
        new DefaultLog(QS::QingStor::Configure::GetLogDirectory()));
    InitializeGLog();
  });
  return logInstance.get();
}

void Log::SetLogLevel(LogLevel level) {
  m_logLevel = level;
  FLAGS_minloglevel = static_cast<int>(level);
}

void ConsoleLog::Initialize() { FLAGS_logtostderr = 1; }

void DefaultLog::Initialize() {
  // Notes for glog, most settings start working immediately after you upate
  // FLAGS_*. The exceptions are the flags related to desination files.
  // So need to set FLAGS_log_dir before calling google::InitGoogleLogging.
  FLAGS_log_dir = m_path.c_str();

  if (!QS::Utils::CreateDirectoryIfNotExistsNoLog(m_path)) {
    throw QSException("Unable to create log directory " + m_path + ".");
  }

  auto outcome = QS::Utils::DeleteFilesInDirectoryNoLog(m_path, false);
  if (!outcome.first) {
    std::cerr << outcome.second << "But Continue..." << std::endl;
  }
}

}  // namespace Logging
}  // namespace QS
