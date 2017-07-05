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

#ifndef _QSFS_FUSE_INCLUDE_BASE_UTILS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_UTILS_H_  // NOLINT

#include <stdio.h>

#include <string>
#include <type_traits>

namespace QS {

namespace Utils {

struct EnumHash {
  template <typename T>
  int operator()(T enumValue) const {
    return static_cast<int>(enumValue);
  }
};

struct StringHash {
  int operator()(const std::string &strToHash) const {
    if (strToHash.empty()) {
      return 0;
    }

    int hash = 0;
    for (const auto &charValue : strToHash) {
      hash = charValue + 31 * hash;
    }
    return hash;
  }
};

template <typename P>
std::string PointerAddress(P p) {
  if (std::is_pointer<P>::value) {
    int sz = snprintf(NULL, 0, "%p", p);
    char *buf = new char[sz + 1];
    snprintf(buf, sz + 1, "%p", p);
    std::string ss(buf);
    delete[] buf;
    return ss;
  }
  return std::string();
}

}  // namespace Utils
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_UTILS_H_
