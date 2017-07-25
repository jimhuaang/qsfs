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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_URI_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_URI_H_  // NOLINT

#include <stdint.h>

#include <string>

#include "client/Protocol.h"

namespace QS {

namespace Client {

namespace Http {

enum class Host {
  Null,
  QingStor
  // Add other hosts here
  // AWS
};

std::string HostToString(Host host);
Host StringToHost(const std::string &name);
Host GetDefaultHost();
std::string GetDefaultHostName();

uint16_t GetDefaultPort(Protocol protocol);

}  // namespace Http

}  // namespace Client
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_URI_H_
