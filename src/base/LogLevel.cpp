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

#include "base/LogLevel.h"

#include <unordered_map>

#include "base/Utils.h"

namespace QS {

namespace Logging {

using QS::Utils::EnumHash;
using QS::Utils::StringHash;
using std::string;
using std::unordered_map;

const string &GetLogLevelName(LogLevel logLevel) {
  static unordered_map<LogLevel, string, EnumHash> logLevelNames = {
      {LogLevel::Info, "INFO"},
      {LogLevel::Warn, "WARN"},
      {LogLevel::Error, "ERROR"},
      {LogLevel::Fatal, "FATAL"}};
  return logLevelNames[logLevel];
}

LogLevel GetLogLevelByName(const std::string &name) {
  static unordered_map<string, LogLevel, StringHash> nameLogLevels = {
      {"info", LogLevel::Info},
      {"Info", LogLevel::Info},
      {"INFO", LogLevel::Info},
      {"warn", LogLevel::Warn},
      {"Warn", LogLevel::Warn},
      {"WARN", LogLevel::Warn},
      {"warning", LogLevel::Warn},
      {"Warning", LogLevel::Warn},
      {"WARNING", LogLevel::Warn},
      {"error", LogLevel::Error},
      {"Error", LogLevel::Error},
      {"ERROR", LogLevel::Error},
      {"fatal", LogLevel::Fatal},
      {"Fatal", LogLevel::Fatal},
      {"FATAL", LogLevel::Fatal}
      // Add other entries here
  };

  auto it = nameLogLevels.find(name);
  return it != nameLogLevels.end() ? it->second : LogLevel::Info;
}

string GetLogLevelPrefix(LogLevel logLevel) {
  return "[" + GetLogLevelName(logLevel) + "] ";
}

}  // namespace Logging
}  // namespace QS
