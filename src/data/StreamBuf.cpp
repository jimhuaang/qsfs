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

#include "data/StreamBuf.h"

#include <assert.h>

#include <string>
#include <utility>

#include "base/LogMacros.h"

namespace QS {

namespace Data {

using std::shared_ptr;
using std::to_string;
using std::vector;

StreamBuf::StreamBuf(Buffer buf, size_t lengthToRead)
    : m_buffer(std::move(buf)), m_lengthToRead(lengthToRead) {
  assert(m_buffer);
  DebugFatalIf(!m_buffer,
               "Try to initialize streambuf with null preallocated buffer");
  auto buffSize = m_buffer->size();

  bool rightStatus = m_lengthToRead <= buffSize;
  assert(rightStatus);
  DebugFatalIf(!rightStatus,
               "Streambuf only have a " + to_string(buffSize) +
                   " bytes buffer, but want stream to see " +
                   to_string(m_lengthToRead) + " bytes of it");

  setp(begin(), end());
  setg(begin(), begin(), end());
}

StreamBuf::~StreamBuf() {
  if (m_buffer) {
    m_buffer.reset();
  }
}

StreamBuf::pos_type StreamBuf::seekoff(off_type off, std::ios_base::seekdir dir,
                                       std::ios_base::openmode which) {
  if (dir == std::ios_base::beg) {
    return seekpos(off, which);
  } else if (dir == std::ios_base::end) {
    return seekpos(m_lengthToRead - off, which);
  } else if (dir == std::ios_base::cur) {
    if (which == std::ios_base::in) {
      return seekpos((gptr() - begin()) + off, which);
    } else if (which == std::ios_base::out) {
      return seekpos((pptr() - begin()) + off, which);
    }
  }
  return off_type(-1);
}

StreamBuf::pos_type StreamBuf::seekpos(pos_type pos,
                                       std::ios_base::openmode which) {
  auto szPos = static_cast<size_t>(pos);
  bool seekAllowed = szPos <= m_lengthToRead;
  assert(seekAllowed);
  DebugErrorIf(
      !seekAllowed,
      "Streambuf only allow stream to see " + to_string(m_lengthToRead) +
          " bytes, but try to seek to buffer position " + to_string(szPos));

  DebugErrorIf(!m_buffer, "Streambuf has null buffer");

  if (!m_buffer && szPos > m_lengthToRead) {
    return pos_type(off_type(-1));
  }

  if (which == std::ios_base::in) {
    setg(begin(), begin() + szPos, end());
  } else if (which == std::ios_base::out) {
    setp(begin() + szPos, end());
  }
  return pos;
}

Buffer StreamBuf::ReleaseBuffer() {
  if (m_buffer) {
    auto bufToRelease = std::move(m_buffer);
    m_buffer = nullptr;
    return bufToRelease;
  } else {
    return nullptr;
  }
}

}  // namespace Data
}  // namespace QS
