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

#ifndef _QSFS_FUSE_INCLUDED_CLIENT_UTILS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_CLIENT_UTILS_H_  // NOLINT

#include <stddef.h>  // for size_t

#include <string>
#include <tuple>

#include <sys/types.h>  // for off_t

namespace QS {

namespace Client {

namespace Utils {

static const char *CONTENT_TYPE_STREAM1 = "application/octet-stream";
static const char *CONTENT_TYPE_STREAM2 = "binary/octet-stream";
static const char *CONTENT_TYPE_DIR = "application/x-directory";
static const char *CONTENT_TYPE_TXT = "text/plain";
// TODO(jim): refer s3fs::get_mode, content-type to file type

// Build request header of 'Range'
// format of "bytes=start_offset-stop_offset"
std::string BuildRequestRange(off_t start, size_t size);
// format of "bytes=start_offset-"
std::string BuildRequestRangeStart(off_t start);
// format of "bytes=-suffix_length"
std::string BuildRequestRangeEnd(off_t suffixLen);

// Parse response header of 'Content-Range' or 'Content-Copy-Range'
// which has format of "bytes start_offset-stop_offset/file_size"
// Return start, body length(stop - start + 1), total file_size
std::tuple<off_t, size_t, size_t> ParseResponseContentRange(
    const std::string &range);

}  // namespace Utils
}  // namespace Client
}  // namespace QS


// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_CLIENT_UTILS_H_
