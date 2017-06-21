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
Log *GetLogInstance();

class Log {
 public:
  Log() = default;
  virtual ~Log() = default;

 protected:
  virtual void Initialize() = 0;

};

class ConsoleLog : public Log {
 public:
  using BASE = Log;

  ConsoleLog() : BASE() { Initialize(); }

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

  DefaultLog(const std::string &path = "")
      : BASE(), m_path(path) {
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

}  // namespace Logging
}  // namespace QS

#endif  // LOGGING_H_INCLUDED
