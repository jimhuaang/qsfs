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

#include <client/QSErrors.h>

#include <unordered_map>

namespace QS {

namespace Client {

using std::string;
using std::unordered_map;

Error<QSErrors> GetQSErrorForCode(const string &errorCode) {
  static unordered_map<string, QSErrors, StringHash> errorCodeToTypeMap = {
      {"invalid_access_key_id", QSErrors::INVALID_ACCESS_KEY_ID},
      {"invalid_range",         QSErrors::INVALID_RANGE},
      // TODO: add other errors here
  };

  auto it = errorCodeToTypeMap.find(errorCode);
  return it != errorCodeToTypeMap.end()
             ? Error<QSErrors>(it->second, false)
             : Error<QSErrors>(QSErrors::UNKNOWN, false);
}

Error<QSErrors> GetQSErrorForCode(const char *errorCode) {
  string str = errorCode ? string(errorCode) : string();
  return GetQSErrorForCode(str);
}

}  // namespace Client
}  // namespace QS
