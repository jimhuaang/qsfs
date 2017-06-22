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

#include <memory>
#include <mutex>  // NOLINT

#include "glog/logging.h"

#include "base/LogLevel.h"

namespace {

void InitializeGLog() { google::InitGoogleLogging("QSFS"); }

}  // namespace

namespace QS {

namespace Logging {

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
    // TODO(Jim): Change to default path.
    logInstance = unique_ptr<Log>(new DefaultLog(""));
    InitializeGLog();
  });
  return logInstance.get();
}

void ConsoleLog::Initialize() { FLAGS_logtostderr = 1; }

void DefaultLog::Initialize() { FLAGS_log_dir = m_path.c_str(); }

}  // namespace Logging

}  // namespace QS
