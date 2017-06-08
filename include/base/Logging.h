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

#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

#include <atomic>
#include <memory>

#include <base/LogLevel.h>

namespace QS {

namespace Logging {
class Log;

void InitializeLogging(std::unique_ptr<Log> log);
void ShutdownLogging();
Log *GetLogInstance();

class Log {
 public:
  Log(LogLevel logLevel) : m_logLevel(logLevel) {}

  virtual ~Log() = default;

 public:
  void LogMessage(LogLevel logLevel, const std::string &msg);
  void LogMessageIf(LogLevel logLevel, bool condition, const std::string &msg);

  /* Log message in "debug mode" (i.e., there is no NDEBUG macro defined)*/
  void DebugLogMessage(LogLevel logLevel, const std::string &msg);
  void DebugLogMessageIf(LogLevel logLevel, bool condition,
                         const std::string &msg);

  LogLevel GetLogLevel() const { return m_logLevel; }
  void SetLogLevel(LogLevel logLevel) { m_logLevel.store(logLevel); }

 protected:
  virtual void Initialize() = 0;

 private:
  std::atomic<LogLevel> m_logLevel;
};

class ConsoleLog : public Log {
 public:
  using BASE = Log;

  ConsoleLog(LogLevel logLevel) : BASE(logLevel) { Initialize(); }

  ConsoleLog(ConsoleLog &&) = delete;
  ConsoleLog(const ConsoleLog &) = delete;
  ConsoleLog &operator=(ConsoleLog &&) = delete;
  ConsoleLog &operator=(const ConsoleLog &) = delete;
  virtual ~ConsoleLog() = default;

 protected:
  virtual void Initialize() override;
};

class DefaultLog : public Log {
 public:
  using BASE = Log;

  DefaultLog(LogLevel logLevel, const std::string &path)
      : BASE(logLevel), m_path(path) {
    Initialize();
  }

  DefaultLog(DefaultLog &&) = delete;
  DefaultLog(const DefaultLog &) = delete;
  DefaultLog &operator=(DefaultLog &&) = delete;
  DefaultLog &operator=(const DefaultLog &) = delete;
  virtual ~DefaultLog() = default;

 protected:
  virtual void Initialize() override;

 private:
  std::string m_path;
};

} // namespace Logging
} // namespace QS

#endif //LOGGING_H_INCLUDED
