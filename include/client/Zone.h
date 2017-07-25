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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_ZONE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_ZONE_H_  // NOLINT

#include <string>

namespace QS {

namespace Client {

namespace QSZone {
// TODO(jim): make sure which zones are available
static const char *const AP_1 = "ap1";
static const char *const PEK_2 = "pek2";
static const char *const PEK_3A = "pek3a";
static const char *const GD_1 = "gd1";
static const char *const GD_2A = "gd2a";
static const char *const SH_1A = "sh1a";
}  // namespace Zone

// TODO(jim): probably not needed
std::string FromEndPoint(const std::string &zoneName);

const char *GetDefaultZone();

}  // namespace Client
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDED_CLIENT_ZONE_H_
