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

#include <sys/stat.h>

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "base/LogMacros.h"
#include "base/Logging.h"
#include "base/Utils.h"

namespace {

using QS::Logging::DefaultLog;
using QS::Logging::Log;
using std::fstream;
using std::string;
using std::unique_ptr;
using std::vector;
using ::testing::Values;
using ::testing::WithParamInterface;

static const char *defaultLogDir = "/tmp/qsfs.logs";

void MakeDefaultLogDir() {
  auto success = QS::Utils::CreateDirectoryIfNotExistsNoLog(defaultLogDir);
  ASSERT_TRUE(success) << "Fail to create directory " << defaultLogDir << ".";
}

void LogNonFatalPossibilities() {
  Info("test Info");
  Warning("test Warning");
  Error("test Error");
  InfoIf(true, "test InfoIf");
  WarningIf(true, "test WarningIf");
  ErrorIf(true, "test ErrorIf");
  InfoIf(false, "test InfoIf");
  WarningIf(false, "test WarningIf");
  ErrorIf(false, "test ErrorIf");
  DebugInfo("test DebugInfo");
  DebugWarning("test DebugWarning");
  DebugError("test DebugError");
  DebugInfoIf(true, "test DebugInfoIf");
  DebugWarningIf(true, "test DebugWarningIf");
  DebugErrorIf(true, "test DebugErrorIf");
  DebugInfoIf(false, "test DebugInfoIf");
  DebugWarningIf(false, "test DebugWarningIf");
  DebugErrorIf(false, "test DebugErrorIf");
}

// As glog will handle the order of severity level, So only need to verify the
// lowest severity level (i.e., INFO).
void VerifyAllNonFatalLogs() {
  string infoLogFile = string(defaultLogDir) + "/QSFS.INFO";
  struct stat info;
  int status = stat(infoLogFile.c_str(), &info);
  ASSERT_EQ(status, 0) << infoLogFile << " is not existed.";

  vector<string> logMsgs;
  {
    fstream fs(infoLogFile);
    ASSERT_TRUE(fs.is_open()) << "Fail to open " << infoLogFile << ".";

    string::size_type pos = string::npos;
    for (string line; std::getline(fs, line);) {
      if ((pos = line.find("[INFO]")) != string::npos ||
          (pos = line.find("[WARN]")) != string::npos ||
          (pos = line.find("[ERROR]")) != string::npos) {
        logMsgs.emplace_back(line, pos);
      }
    }
  }

  vector<string> expectedMsgs = {
      "[INFO] test Info",           "[WARN] test Warning",
      "[ERROR] test Error",         "[INFO] test InfoIf",
      "[WARN] test WarningIf",      "[ERROR] test ErrorIf",
      "[INFO] test DebugInfo",      "[WARN] test DebugWarning",
      "[ERROR] test DebugError",    "[INFO] test DebugInfoIf",
      "[WARN] test DebugWarningIf", "[ERROR] test DebugErrorIf"};

  EXPECT_EQ(logMsgs, expectedMsgs);
}

class LoggingTest : public ::testing::Test {
 public:
  LoggingTest() {
    QS::Logging::InitializeLogging(
        unique_ptr<Log>(new DefaultLog(defaultLogDir)));
  }

  void SetUp() override { MakeDefaultLogDir(); }

  ~LoggingTest() {}
};

// As glog logging a FATAL message will terminate the program,
// so add death tests for FATAL log.
using LogFatalFun = std::function<void()>;

struct LogFatalState {
  LogFatalFun logFatalFunc;
  string fatalMsg;
};

class LoggingDeathTest : public LoggingTest,
                         public WithParamInterface<LogFatalState> {
 public:
  void SetUp() override {
    MakeDefaultLogDir();
    m_fatalMsg = GetParam().fatalMsg;
  }

 protected:
  string m_fatalMsg;
};

void LogFatal() { Fatal("test Fatal"); }
void LogFatalIf() { FatalIf(true, "test FatalIf"); }
void LogDebugFatal() { DebugFatal("test DebugFatal"); }
void LogDebugFatalIf() { DebugFatalIf(true, "test DebugFatalIf"); }

void VerifyFatalLog(const string &expectedMsg) {
  string fatalLogFile = string(defaultLogDir) + "/QSFS.FATAL";
  struct stat info;
  int status = stat(fatalLogFile.c_str(), &info);
  ASSERT_EQ(status, 0) << fatalLogFile << " is not existed.";

  string logMsg;
  {
    fstream fs(fatalLogFile);
    ASSERT_TRUE(fs.is_open()) << "Fail to open " << fatalLogFile << ".";

    string::size_type pos = string::npos;
    for (string line; std::getline(fs, line);) {
      if ((pos = line.find("[FATAL]")) != string::npos) {
        logMsg = string(line, pos);
        break;
      }
    }
  }
  EXPECT_EQ(logMsg, expectedMsg);
}

}  // namespace

TEST_F(LoggingTest, TestAllNonFatalLogs) {
  LogNonFatalPossibilities();
  VerifyAllNonFatalLogs();
}

TEST_P(LoggingDeathTest, TestLogFatal) {
  auto func = GetParam().logFatalFunc;
  ASSERT_DEATH({ func(); }, "");
  VerifyFatalLog(m_fatalMsg);
}

INSTANTIATE_TEST_CASE_P(
    LogFatal, LoggingDeathTest,
    Values(LogFatalState{LogFatal, "[FATAL] test Fatal"},
           LogFatalState{LogFatalIf, "[FATAL] test FatalIf"},
           LogFatalState{LogDebugFatal, "[FATAL] test DebugFatal"},
           LogFatalState{LogDebugFatalIf, "[FATAL] test DebugFatalIf"}));

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
