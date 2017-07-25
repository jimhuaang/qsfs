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

#include "base/StringUtils.h"

#include <algorithm>
#include <cctype>

namespace QS {

namespace StringUtils {

using std::string;

string ToLower(const string &str) {
  string copy(str);
  for (auto &c : copy) {
    c = std::tolower(c);
  }
  return copy;
}

string ToUpper(const string &str) {
  string copy(str);
  for (auto &c : copy) {
    c = std::toupper(c);
  }
  return copy;
}

string LTrim(const string &str) {
  string copy(str);
  auto pos = std::find_if_not(copy.begin(), copy.end(),
                              [](unsigned char c) { return std::isspace(c); });
  copy.erase(copy.begin(), pos);
  return copy;
}

string RTrim(const string &str) {
  string copy(str);
  auto rpos = std::find_if_not(copy.rbegin(), copy.rend(),
                               [](unsigned char c) { return std::isspace(c); });
  copy.erase(rpos.base(), copy.end());
  return copy;
}

string Trim(const string &str) { return LTrim(RTrim(str)); }

}  // namespace StringUtils
}  // namespace QS
