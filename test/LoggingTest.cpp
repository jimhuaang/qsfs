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
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "base/LogMacros.h"
#include "base/Logging.h"

namespace {

using namespace QS::Logging;

using std::fstream;
using std::string;
using std::unique_ptr;
using std::vector;

static const string defaultLogDir = "/tmp/qsfs/logs/test";

void MakeDefaultLogDir() {
  struct stat info;
  int status = stat(defaultLogDir.c_str(), &info);

  if (status == 0) {
    ASSERT_TRUE(((info.st_mode) & S_IFMT) == S_IFDIR);
  } else {
    int status = mkdir(defaultLogDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
    ASSERT_EQ(status, 0);
  }
}

// Not include the FATAL severity level as logging a FATAL message will
// terminate the program.
void LogAllPossibilities() {
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
void VerifyAllLogs() {
  string infoLogFile = defaultLogDir + "/QSFS.INFO";
  struct stat info;
  int status = stat(infoLogFile.c_str(), &info);
  ASSERT_EQ(status, 0);

  vector<string> logMsgs;
  {
    fstream fs(infoLogFile);
    ASSERT_TRUE(fs.is_open());

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
    InitializeLogging(unique_ptr<Log>(new DefaultLog(defaultLogDir)));
  }

  virtual void SetUp() override { MakeDefaultLogDir(); }

  ~LoggingTest() {}
};

}  // namespace

TEST_F(LoggingTest, DefaultLogTest) {
  LogAllPossibilities();
  VerifyAllLogs();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
