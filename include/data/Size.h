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

#ifndef _QSFS_FUSE_INCLUDED_DATA_SIZE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_SIZE_H_  // NOLINT

#include <stddef.h>  // for size_t
#include <stdint.h>  // for unit64_t

namespace QS {

namespace Data {

namespace Size {

const static uint64_t MB5 = 5 * 1024 * 1024;
const static uint64_t MB10 = 10 * 1024 * 1024;
const static uint64_t MB20 = 20 * 1024 * 1024;
const static uint64_t MB50 = 50 * 1024 * 1024;
const static uint64_t MB100 = 100 * 1024 * 1024;
const static uint64_t GB1 = 1024 * 1024 * 1024;

const static size_t K1 = 1 * 1000;
const static size_t K10 = 10 * 1000;
const static size_t K100 = 100 * 1000;
const static size_t M1 = 1* 1000 * 1000;
const static size_t M10 = 10 * 1000 * 1000;
const static size_t M100 = 100 * 1000 * 1000;

}  // namespace Size
}  // namespace Data
}  // namespace QS


// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_DATA_SIZE_H_
