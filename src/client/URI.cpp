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

#include "client/URI.h"

#include <string>
#include <unordered_map>

#include "base/HashUtils.h"
#include "base/LogMacros.h"

namespace QS {

namespace Client {

namespace Http {

using QS::HashUtils::EnumHash;
using QS::HashUtils::StringHash;
using std::string;
using std::to_string;
using std::unordered_map;

static const char* const HOST_QINGSTOR = "qingstor.com";
static const char* const HOST_NULL = "";
// static const char* const HOST_AWS = "s3.amazonaws.com";

std::string HostToString(Host host) {
  static unordered_map<Host, string, EnumHash> hostToNameMap = {
      {Host::Null, HOST_NULL}, {Host::QingStor, HOST_QINGSTOR}
      // Add other entries here
  };
  auto it = hostToNameMap.find(host);
  if (it == hostToNameMap.end()) {
    DebugWarning(
        "Trying to get host name with unrecognized host type, null returned");
    return HOST_NULL;
  }
  return it->second;
}

Host StringToHost(const string& name) {
  static unordered_map<string, Host, StringHash> nameToHostMap = {
      {HOST_NULL, Host::Null}, {HOST_QINGSTOR, Host::QingStor}
      // Add other entries here
  };
  auto it = nameToHostMap.find(name);
  if (it == nameToHostMap.end()) {
    DebugWarning(
        "Trying to get host with unrecognized host name, null returned");
    return Host::Null;
  }
  return it->second;
}

}  // namespace Http
}  // namespace Client
}  // namespace QS
