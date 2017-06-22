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

string GetLogLevelPrefix(LogLevel logLevel) {
  return "[" + GetLogLevelName(logLevel) + "] ";
}

}  // namespace Logging
}  // namespace QS
