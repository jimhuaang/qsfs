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

#include "data/Page.h"

#include <assert.h>

#include <sstream>
#include <string>

#include "base/LogMacros.h"
#include "base/StringUtils.h"  //
#include "data/IOStream.h"
#include "data/StreamUtils.h"

namespace QS {

namespace Data {

using QS::Data::IOStream;
using QS::StringUtils::PointerAddress;
using std::make_shared;
using std::iostream;
using std::shared_ptr;
using std::stringstream;
using std::string;
using std::to_string;

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const char *buffer)
    : m_offset(offset), m_size(len), m_body(make_shared<IOStream>(len)) {
  bool isValidInput = offset >= 0 && len >= 0 && buffer != NULL;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len, buffer));
    return;
  }
  assert(m_body);
  if (!m_body) {
    DebugError("Try to new a page with a null stream body");
  }

  m_body->seekp(0, std::ios_base::beg);
  m_body->write(buffer, len);
  DebugErrorIf(
      m_body->fail(),
      "Fail to new a page from buffer " + ToStringLine(offset, len, buffer));
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const shared_ptr<iostream> &instream)
    : m_offset(offset), m_size(len), m_body(make_shared<IOStream>(len)) {
  bool isValidInput = offset >= 0 && len > 0 && instream;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len));
    return;
  }
  assert(m_body);
  if (!m_body) {
    DebugError("Try to new a page with a null stream body");
  }

  size_t instreamLen = QS::Data::StreamUtils::GetStreamSize(instream);

  if (instreamLen < len) {
    DebugWarning(
        "Input stream buffer len is less than parameter 'len', Ajust it");
    len = instreamLen;
  }

  instream->seekg(0, std::ios_base::beg);
  m_body->seekp(0, std::ios_base::beg);
  if (len == instreamLen) {
    (*m_body) << instream->rdbuf();
  } else {  // len > instreamLen
    stringstream ss;
    ss << instream->rdbuf();
    m_body->write(ss.str().c_str(), len);
  }
  DebugErrorIf(m_body->fail(),
               "Fail to new a page from stream " + ToStringLine(offset, len));
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, shared_ptr<iostream> &&body)
    : m_offset(offset), m_size(len), m_body(std::move(body)) {}

// --------------------------------------------------------------------------
void Page::SetStream(shared_ptr<iostream> &&stream) {
  m_body = std::move(stream);
}

// --------------------------------------------------------------------------
bool Page::Refresh(off_t offset, size_t len, const char *buffer) {
  bool isValidInput = offset >= m_offset && buffer != NULL && len > 0;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to refresh page(" + ToStringLine(m_offset, m_size) +
               ") with invalid input " + ToStringLine(offset, len, buffer));
    return false;
  }
  return UnguardedRefresh(offset, len, buffer);
}

// --------------------------------------------------------------------------
bool Page::UnguardedRefresh(off_t offset, size_t len, const char *buffer) {
  m_body->seekp(offset - m_offset, std::ios_base::beg);
  m_body->write(buffer, len);
  auto moreLen = offset + len - Next();
  if (moreLen > 0) {
    m_size += moreLen;
  }

  auto success = m_body->good();
  DebugErrorIf(!success,
               "Fail to refresh page(" + ToStringLine(m_offset, m_size) +
                   ") with input " + ToStringLine(offset, len, buffer));
  return success;
}

// --------------------------------------------------------------------------
void Page::Resize(size_t smallerSize) {
  // Do a lazy resize:
  // 1. Change size to 'samllerSize'.
  // 2. Set output position indicator to 'samllerSize'.
  assert(0 <= smallerSize && smallerSize <= m_size);
  m_size = smallerSize;
  m_body->seekp(smallerSize, std::ios_base::beg);
}

// --------------------------------------------------------------------------
size_t Page::Read(off_t offset, size_t len, char *buffer) {
  bool isValidInput = (offset >= m_offset && buffer != NULL && len > 0 &&
                       len <= static_cast<size_t>(Next() - offset));
  assert(isValidInput);
  DebugErrorIf(!isValidInput,
               "Try to read page (" + ToStringLine(m_offset, m_size) +
                   ") with invalid input " + ToStringLine(offset, len, buffer));
  return UnguardedRead(offset, len, buffer);
}

// --------------------------------------------------------------------------
size_t Page::UnguardedRead(off_t offset, size_t len, char *buffer) {
  m_body->seekg(offset - m_offset, std::ios_base::beg);
  m_body->read(buffer, len);
  DebugErrorIf(!m_body->good(),
               "Fail to read page(" + ToStringLine(m_offset, m_size) +
                   ") with input " + ToStringLine(offset, len, buffer));
  return len;
}

// --------------------------------------------------------------------------
string ToStringLine(off_t offset, size_t len, const char *buffer) {
  return "[offset:size:buffer] = [" + to_string(offset) + ":" + to_string(len) +
         ":" + PointerAddress(buffer) + "]";
}

// --------------------------------------------------------------------------
string ToStringLine(off_t offset, size_t size) {
  return "[offset:size] = [" + to_string(offset) + ":" + to_string(size) + "]";
}

// --------------------------------------------------------------------------
string ToStringLine(const string &fileId, off_t offset, size_t len,
                    const char *buffer) {
  return "[fileId:offset:size:buffer] = [" + fileId + ":" + to_string(offset) +
         ":" + to_string(len) + ":" + PointerAddress(buffer) + "]";
}

}  // namespace Data
}  // namespace QS
