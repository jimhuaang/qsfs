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

#include "client/Zone.h"

#include <string>
#include <unordered_map>

#include "base/HashUtils.h"
#include "base/LogMacros.h"

namespace QS {

namespace Client {

using std::string;
using std::unordered_map;

string FromEndPoint(const string &zoneName) {
  if (zoneName.empty()) {
    DebugError("Try to get end point with empty zone name");
    return string();
  }

  unordered_map<string, string, QS::HashUtils::StringHash>
      zoneNameToEndPointMap = {{QSZone::AP_1, "ap1.qingstor.com"},
                               {QSZone::PEK_2, "pek2.qingstor.com"},
                               {QSZone::PEK_3A, "pek3a.qingstor.com"},
                               {QSZone::GD_1, "gd1.qingstor.com"},
                               {QSZone::GD_2A, "gd2a.qingstor.com"},
                               {QSZone::SH_1A, "sh1a.qingstor.com"}};

  auto it = zoneNameToEndPointMap.find(zoneName);
  if (it != zoneNameToEndPointMap.end()) {
    return it->second;
  } else {
    DebugError("Try to get end point with an unrecognized zone name " +
               zoneName);
    return string();
  }
}

const char *GetDefaultZone() { return QSZone::PEK_3A; }

}  // namespace Client
}  // namespace QS
