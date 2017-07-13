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

#ifndef _QSFS_FUSE_INCLUDED_QINGSTOR_UTILS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_QINGSTOR_UTILS_H_  // NOLINT

#include <sys/stat.h>
#include <sys/types.h>

#include <string>

namespace QS {

namespace QingStor {

namespace Utils {

// Is given uid included in group of gid
std::string GetUserName(uid_t uid);
bool IsIncludedInGroup(uid_t uid, gid_t gid);

bool GetProcessEffectiveUserID(uid_t *uid);
bool GetProcessEffectiveGroupID(gid_t *gid);

bool HavePermission(struct stat &st);
bool HavePermission(const std::string &path);

}  // namespace Utils
}  // namespace QingStor
}  // namespace QS

// NOLINTNEXTLINT
#endif  // _QSFS_FUSE_INCLUDED_QINGSTOR_UTILS_H_
