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

#ifndef _QSFS_FUSE_INCLUDE_BASE_LOGGING_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_LOGGING_H_  // NOLINT

#include <atomic>
#include <memory>
#include <string>

#include "gtest/gtest_prod.h"  // FRIEND_TEST

#include "base/LogLevel.h"

// Declare main in global namespace before class Log, since friend declarations
// can only introduce names in the surrounding namespace.
extern int main(int argc, char **argv);

namespace QS {

namespace Logging {
class Log;

void InitializeLogging(std::unique_ptr<Log> log);
Log *GetLogInstance();

class Log {
 public:
  Log() noexcept = default;
  virtual ~Log() = default;

 public:
  LogLevel GetLogLevel() const noexcept { return m_logLevel; }
  bool IsDebug() const noexcept { return m_isDebug; }

 protected:
  virtual void Initialize() = 0;
  virtual void ClearLogDirectory() const = 0;

 private:
  void SetLogLevel(LogLevel level) noexcept;
  void SetDebug(bool debug) noexcept { m_isDebug = debug; }
  FRIEND_TEST(LoggingTest, TestNonFatalLogsLevelInfo);
  FRIEND_TEST(LoggingTest, TestNonFatalLogsLevelWarn);
  FRIEND_TEST(LoggingTest, TestNonFatalLogsLevelError);
  FRIEND_TEST(FatalLoggingDeathTest, TestWithDebugAndIf);

 private:
  LogLevel m_logLevel = LogLevel::Info;
  bool m_isDebug = false;

  friend int ::main(int argc, char **argv);
};

class ConsoleLog : public Log {
 public:
  using BASE = Log;

  ConsoleLog() noexcept : BASE() { Initialize(); }

  ConsoleLog(ConsoleLog &&) = delete;
  ConsoleLog(const ConsoleLog &) = delete;
  ConsoleLog &operator=(ConsoleLog &&) = delete;
  ConsoleLog &operator=(const ConsoleLog &) = delete;
  virtual ~ConsoleLog() = default;

 protected:
  void Initialize() noexcept override;
  void ClearLogDirectory() const noexcept override;
};

class DefaultLog : public Log {
 public:
  using BASE = Log;

  explicit DefaultLog(const std::string &path = "") : BASE(), m_path(path) {
    Initialize();
  }

  DefaultLog(DefaultLog &&) = delete;
  DefaultLog(const DefaultLog &) = delete;
  DefaultLog &operator=(DefaultLog &&) = delete;
  DefaultLog &operator=(const DefaultLog &) = delete;
  virtual ~DefaultLog() = default;

 protected:
  void Initialize() override;
  void ClearLogDirectory() const override;

 private:
  std::string m_path;
};

}  // namespace Logging
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_LOGGING_H_
