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

#include "client/Utils.h"

#include <assert.h>

#include <iostream>
#include <sstream>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "filesystem/MimeTypes.h"

namespace QS {

namespace Client {

namespace Utils {

using QS::FileSystem::MimeTypes;
using std::istream;
using std::make_tuple;
using std::string;
using std::to_string;
using std::tuple;

template <char C>
istream &expect(istream &in) {
  if ((in >> std::ws).peek() == C) {
    in.ignore();
  } else {
    in.setstate(std::ios_base::failbit);
  }
  return in;
}
template istream &expect<'-'>(istream &in);
template istream &expect<'/'>(istream &in);

static const char *CONTENT_TYPE_STREAM1 = "application/octet-stream";
static const char *CONTENT_TYPE_STREAM2 = "binary/octet-stream";
static const char *CONTENT_TYPE_DIR = "application/x-directory";
static const char *CONTENT_TYPE_TXT = "text/plain";

// --------------------------------------------------------------------------
std::string BuildRequestRange(off_t start, size_t size) {
  assert(size > 0);
  DebugWarningIf(size == 0, "Invalide input with zero range size");
  // format: "bytes=start_offset-stop_offset"
  string range = "bytes=";
  range += to_string(start);
  range += "-";
  range += to_string(start + size - 1);
  return range;
}

// --------------------------------------------------------------------------
string BuildRequestRangeStart(off_t start){
  // format of "bytes=start_offset-"
  string range = "bytes=";
  range += to_string(start);
  range += "-";
  return range;
}

// --------------------------------------------------------------------------
string BuildRequestRangeEnd(off_t suffixLen){
  // format of "bytes=-suffix_length"
  string range = "bytes=-";
  range += to_string(suffixLen);
  return range;
}

// --------------------------------------------------------------------------
tuple<off_t, size_t, size_t> ParseResponseContentRange(
    const std::string &contentRange) {
  string cpy(QS::StringUtils::Trim(contentRange, ' '));
  assert(!cpy.empty());
  if (cpy.empty()) {
    DebugWarning("Invalid input with empty content range");
    return make_tuple(0, 0, 0);
  }

  auto ErrOut = [&cpy]() -> tuple<off_t, size_t, size_t> {
    DebugWarning("Invalid input: " + cpy);
    return make_tuple(0, 0, 0);
  };

  // format: "bytes start_offset-stop_offset/file_size"
  if (cpy.find("bytes ") == string::npos || cpy.find("-") == string::npos ||
      cpy.find("/") == string::npos) {
    return ErrOut();
  }
  cpy = cpy.substr(6);  // remove leading "bytes "
  off_t start = 0;
  off_t stop = 0;
  size_t size = 0;
  std::istringstream in(cpy);
  if (in >> start >> expect<'-'> >> stop >> expect<'/'> >> size) {
    if (!(start >= 0 && stop >= start && size > 0)) {
      return ErrOut();
    }
    return make_tuple(start, stop - start + 1, size);
  } else {
    return ErrOut();
  }
}

// --------------------------------------------------------------------------
string LookupMimeType(const string &path) {
  string defaultMimeType(CONTENT_TYPE_STREAM1);

  string::size_type lastPos = path.find_last_of('.');
  if (lastPos == string::npos) return defaultMimeType;

  // Extract the last extension
  string ext = path.substr(1 + lastPos);
  // If the last extension matches a mime type, return it
  auto mimeType = MimeTypes::Instance().Find(ext);
  if (!mimeType.empty()) return mimeType;

  // Extract the second to last file exstension
  string::size_type firstPos = path.find_first_of('.');
  if (firstPos == lastPos) return defaultMimeType;  // there isn't a 2nd ext
  string ext2;
  if (firstPos != string::npos && firstPos < lastPos) {
    string prefix = path.substr(0, lastPos);
    // Now get the second to last file extension
    string::size_type nextPos = prefix.find_last_of('.');
    if (nextPos != string::npos) {
      ext2 = prefix.substr(1 + nextPos);
    }
  }
  // If the second extension matches a mime type, return it
  mimeType = MimeTypes::Instance().Find(ext2);
  if (!mimeType.empty()) return mimeType;

  return defaultMimeType;
}

}  // namespace Utils
}  // namespace Client
}  // namespace QS
