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

#include <base/Logging.h>

#include <assert.h>

#include <memory>
#include <mutex>

#include <glog/logging.h>
#include <base/LogLevel.h>

namespace {

void InitializeGLog() {
  google::InitGoogleLogging("QSFS");
}

}  // namespace

namespace QS {

namespace Logging {

using std::once_flag;
using std::string;
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

void ShutdownLogging() {
  if (logInstance) {
    logInstance.reset();
  }
}

Log* GetLogInstance() {
  assert(logInstance);
  if (!logInstance) {
    InitializeLogging(unique_ptr<Log>(new DefaultLog(
        LogLevel::Info, "")));  // TODO : replace with default log dir
  }
  return logInstance.get();
}

void Log::LogMessage(LogLevel logLevel, const string& msg) {
  if (!(GetLogLevel() <= logLevel)) return;

  string msgWithLogLevelPrefix = GetLogLevelPrefix(logLevel) + msg;
  switch (logLevel) {
    case LogLevel::Info:
      LOG(INFO) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Warn:
      LOG(WARNING) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Error:
      LOG(ERROR) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Fatal:
      LOG(FATAL) << msgWithLogLevelPrefix;
      break;
    default:
      assert(0);
      break;
  }
}

void Log::LogMessageIf(LogLevel logLevel, bool condition, const string& msg) {
  if (condition) {
    LogMessage(logLevel, msg);
  }
}

void Log::DebugLogMessage(LogLevel logLevel, const string& msg) {
  if (!(GetLogLevel() <= logLevel)) return;

  string msgWithLogLevelPrefix = GetLogLevelPrefix(logLevel) + msg;
  switch (logLevel) {
    case LogLevel::Info:
      DLOG(INFO) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Warn:
      DLOG(WARNING) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Error:
      DLOG(ERROR) << msgWithLogLevelPrefix;
      break;
    case LogLevel::Fatal:
      DLOG(FATAL) << msgWithLogLevelPrefix;
      break;
    default:
      assert(0);
      break;
  }
}

void Log::DebugLogMessageIf(LogLevel logLevel, bool condition,
                            const string& msg) {
  if (condition) {
    DebugLogMessage(logLevel, msg);
  }
}

void ConsoleLog::Initialize() { FLAGS_logtostderr = 1; }

void DefaultLog::Initialize() { FLAGS_log_dir = m_path.c_str(); }

}  // namespace Logging

}  // namespace QS
