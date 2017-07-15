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

#include <stdio.h>
#include <sys/stat.h>

#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "base/LogLevel.h"
#include "base/LogMacros.h"
#include "base/Logging.h"
#include "base/Utils.h"
#include "qingstor/Configure.h"

namespace QS {

namespace Logging {

// Notice this testing based on the assumption that glog
// always link the qsfs.INFO, qsfs.WARN, qsfs.ERROR,
// qsfs.FATAL to the latest printed log files.

// In order to test Log private members, need to define
// test fixture and tests in the same namespae as Log
// so they can be friends of class Log.

using QS::Logging::LogLevel;
using QS::QingStor::Configure::GetProgramName;
using std::fstream;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;
using ::testing::Values;
using ::testing::WithParamInterface;

static const char *defaultLogDir = "/tmp/qsfs.logs/";
const string infoLogFile = defaultLogDir + string(GetProgramName()) + ".INFO";
const string fatalLogFile = defaultLogDir + string(GetProgramName()) + ".FATAL";

void MakeDefaultLogDir() {
  auto success = QS::Utils::CreateDirectoryIfNotExistsNoLog(defaultLogDir);
  ASSERT_TRUE(success) << "Fail to create directory " << defaultLogDir << ".";
}

void ClearFileContent(const std::string &path) {
  FILE *pf = fopen(path.c_str(), "w");
  if (pf != nullptr) {
    fclose(pf);
  }
}

void LogNonFatalPossibilities() {
  Error("test Error");
  ErrorIf(true, "test ErrorIf");
  DebugError("test DebugError");
  DebugErrorIf(true, "test DebugErrorIf");
  Warning("test Warning");
  WarningIf(true, "test WarningIf");
  DebugWarning("test DebugWarning");
  DebugWarningIf(true, "test DebugWarningIf");
  Info("test Info");
  InfoIf(true, "test InfoIf");
  DebugInfo("test DebugInfo");
  DebugInfoIf(true, "test DebugInfoIf");
}

void VerifyAllNonFatalLogs(LogLevel level) {
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
      "[ERROR] test Error",       "[ERROR] test ErrorIf",
      "[ERROR] test DebugError",  "[ERROR] test DebugErrorIf",
      "[WARN] test Warning",      "[WARN] test WarningIf",
      "[WARN] test DebugWarning", "[WARN] test DebugWarningIf",
      "[INFO] test Info",         "[INFO] test InfoIf",
      "[INFO] test DebugInfo",    "[INFO] test DebugInfoIf"};

  auto RemoveLastLines = [&expectedMsgs](int count) {
    for (int i = 0; i < count && (!expectedMsgs.empty()); ++i) {
      expectedMsgs.pop_back();
    }
  };

  if (level == LogLevel::Warn) {
    RemoveLastLines(4);
  } else if (level == LogLevel::Error) {
    RemoveLastLines(8);
  }

  EXPECT_EQ(logMsgs, expectedMsgs);
}

class LoggingTest : public ::testing::Test {
 public:
  LoggingTest() {}

  static void SetUpTestCase() {
    MakeDefaultLogDir();
    QS::Logging::InitializeLogging(
        unique_ptr<Log>(new DefaultLog(defaultLogDir)));
    EXPECT_TRUE(GetLogInstance() != nullptr) << "log instance is null.";
  }

  static void TearDownTestCase() {}

  ~LoggingTest() {}
};

// As glog logging a FATAL message will terminate the program,
// so add death tests for FATAL log.
using LogFatalFun = std::function<void(bool)>;

struct LogFatalState {
  LogFatalFun logFatalFunc;
  string fatalMsg;
  bool condition;  // only effective for *If macros
  bool isDebug;    // only effective for Debug* macros
  bool willDie;    // this only for death test
  friend ostream &operator<<(ostream &os, const LogFatalState &state) {
    return os << "[fatalMsg: " << state.fatalMsg
              << ", condition: " << std::boolalpha << state.condition
              << ", debug: " << state.isDebug << ", will die: " << state.willDie
              << "]";
  }
};

class FatalLoggingDeathTest : public LoggingTest,
                              public WithParamInterface<LogFatalState> {
 public:
  void SetUp() override {
    MakeDefaultLogDir();
    m_fatalMsg = GetParam().fatalMsg;
  }

 protected:
  string m_fatalMsg;
};

void LogFatal(bool condition) { Fatal("test Fatal"); }
void LogFatalIf(bool condition) { FatalIf(condition, "test FatalIf"); }
void LogDebugFatal(bool condition) { DebugFatal("test DebugFatal"); }
void LogDebugFatalIf(bool condition) {
  DebugFatalIf(condition, "test DebugFatalIf");
}

void VerifyFatalLog(const string &expectedMsg) {
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

// Test Cases
TEST_F(LoggingTest, TestNonFatalLogsLevelInfo) {
  GetLogInstance()->SetDebug(true);
  GetLogInstance()->SetLogLevel(LogLevel::Info);
  ClearFileContent(infoLogFile);  // make sure only contain logs of this test
  LogNonFatalPossibilities();
  VerifyAllNonFatalLogs(LogLevel::Info);
}

TEST_F(LoggingTest, TestNonFatalLogsLevelWarn) {
  GetLogInstance()->SetDebug(true);
  GetLogInstance()->SetLogLevel(LogLevel::Warn);
  ClearFileContent(infoLogFile);  // make sure only contain logs of this test
  LogNonFatalPossibilities();
  VerifyAllNonFatalLogs(LogLevel::Warn);
}

TEST_F(LoggingTest, TestNonFatalLogsLevelError) {
  GetLogInstance()->SetDebug(true);
  GetLogInstance()->SetLogLevel(LogLevel::Error);
  ClearFileContent(infoLogFile);  // make sure only contain logs of this test
  LogNonFatalPossibilities();
  VerifyAllNonFatalLogs(LogLevel::Error);
}

TEST_P(FatalLoggingDeathTest, TestWithDebugAndIf) {
  auto func = GetParam().logFatalFunc;
  auto condition = GetParam().condition;
  GetLogInstance()->SetDebug(GetParam().isDebug);
  // only when log msg fatal sucessfully the test will die,
  // otherwise the test will fail to die.
  if (GetParam().willDie) {
    ASSERT_DEATH({ func(condition); }, "");
  }
  VerifyFatalLog(m_fatalMsg);
}

INSTANTIATE_TEST_CASE_P(
    LogFatal, FatalLoggingDeathTest,
    // Notice when macro fail to log message, glog will not flush any stream
    // to the log file, so the expectMsg will keep unchanged.

    // logFun, expectMsg, condition, isDebug, will die
    Values(LogFatalState{LogFatal, "[FATAL] test Fatal", true, false, true},
           LogFatalState{LogFatalIf, "[FATAL] test FatalIf", true, false, true},
           LogFatalState{LogFatalIf, "[FATAL] test FatalIf", false, false,
                         false},
           LogFatalState{LogDebugFatal, "[FATAL] test DebugFatal", true, true,
                         true},
           LogFatalState{LogDebugFatal, "[FATAL] test DebugFatal", true, false,
                         false},
           LogFatalState{LogDebugFatalIf, "[FATAL] test DebugFatalIf", true,
                         true, true},
           LogFatalState{LogDebugFatalIf, "[FATAL] test DebugFatalIf", false,
                         true, false},
           LogFatalState{LogDebugFatalIf, "[FATAL] test DebugFatalIf", true,
                         false, false},
           LogFatalState{LogDebugFatalIf, "[FATAL] test DebugFatalIf", false,
                         false, false}));

}  // namespace Logging
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
