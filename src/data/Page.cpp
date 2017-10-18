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

#include <fstream>
#include <sstream>
#include <string>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "data/IOStream.h"
#include "data/StreamUtils.h"
#include "filesystem/Configure.h"

namespace QS {

namespace Data {

using QS::Data::IOStream;
using QS::Data::StreamUtils::GetStreamSize;
using QS::FileSystem::Configure::GetCacheTemporaryDirectory;
using QS::StringUtils::FormatPath;
using QS::StringUtils::PointerAddress;
using QS::Utils::CreateDirectoryIfNotExists;
using QS::Utils::GetDirName;
using std::fstream;
using std::make_shared;
using std::iostream;
using std::shared_ptr;
using std::stringstream;
using std::string;
using std::to_string;

namespace {
bool IsTempFile(const string &tmpfilePath) {
  return GetDirName(tmpfilePath) == GetCacheTemporaryDirectory();
}
}  // namespace

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const char *buffer)
    : m_offset(offset), m_size(len), m_body(make_shared<IOStream>(len)) {
  bool isValidInput = offset >= 0 && len >= 0 && buffer != nullptr;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len, buffer));
    return;
  }

  UnguardedPutToBody(offset, len, buffer);
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const char *buffer, const string &tmpfile)
    : m_offset(offset), m_size(len), m_tmpFile(tmpfile) {
  bool isValidInput = offset >= 0 && len >= 0 && buffer != nullptr;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len, buffer));
    return;
  }

  if (SetupTempFile()) {
    UnguardedPutToBody(offset, len, buffer);
  }
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

  UnguardedPutToBody(offset, len, instream);
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const shared_ptr<iostream> &instream,
           const string &tmpfile)
    : m_offset(offset), m_size(len), m_tmpFile(tmpfile) {
  bool isValidInput = offset >= 0 && len > 0 && instream;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len));
    return;
  }

  if (SetupTempFile()) {
    UnguardedPutToBody(offset, len, instream);
  };
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, shared_ptr<iostream> &&body)
    : m_offset(offset), m_size(len), m_body(std::move(body)) {
  size_t streamlen = GetStreamSize(m_body);
  if (streamlen < len) {
    DebugWarning(
        "Input stream buffer len is less than parameter 'len', Ajust it");
    m_size = streamlen;
  }
}

// --------------------------------------------------------------------------
bool Page::UseTempFile() { return !m_tmpFile.empty(); }

// --------------------------------------------------------------------------
void Page::UnguardedPutToBody(off_t offset, size_t len, const char *buffer) {
  if (!m_body) {
    DebugError("null body stream " + ToStringLine(offset, len, buffer));
    return;
  }
  if (UseTempFile()) {
    OpenTempFile(std::ios_base::binary | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
  } else {
    m_body->seekp(0, std::ios_base::beg);
  }
  m_body->write(buffer, len);
  if (m_body->good()) {
    CloseTempFile();
  } else {
    DebugError("Fail to write buffer " + ToStringLine(offset, len, buffer));
  }
}

// --------------------------------------------------------------------------
void Page::UnguardedPutToBody(off_t offset, size_t len,
                              const shared_ptr<iostream> &instream) {
  size_t instreamLen = GetStreamSize(instream);
  if (instreamLen < len) {
    DebugWarning(
        "Input stream buffer len is less than parameter 'len', Ajust it");
    len = instreamLen;
    m_size = instreamLen;
  }

  if (UseTempFile()) {
    OpenTempFile(std::ios_base::binary | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
  } else {
    m_body->seekp(0, std::ios_base::beg);
  }
  instream->seekg(0, std::ios_base::beg);
  if (len == instreamLen) {
    (*m_body) << instream->rdbuf();
  } else if (len < instreamLen) {
    stringstream ss;
    ss << instream->rdbuf();
    m_body->write(ss.str().c_str(), len);
  }

  if (m_body->good()) {
    CloseTempFile();
  } else {
    DebugError("Fail to write buffer " + ToStringLine(offset, len));
  }
}

// --------------------------------------------------------------------------
void Page::SetStream(shared_ptr<iostream> &&stream) {
  m_body = std::move(stream);
}

// --------------------------------------------------------------------------
bool Page::SetupTempFile() {
  CreateDirectoryIfNotExists(GetCacheTemporaryDirectory());
  if (!IsTempFile(m_tmpFile)) {
    DebugError("tmp file not under /tmp " + FormatPath(m_tmpFile));
    return false;
  }

  // Notice: set open mode with std::ios_base::out to create file in disk if it
  // is not exist
  auto file = make_shared<fstream>(m_tmpFile,
                                   std::ios_base::binary | std::ios_base::out);
  if (file && *file) {
    DebugInfo("Open tmp file " + FormatPath(m_tmpFile));
    file->close();
    SetStream(std::move(file));
    return true;
  } else {
    DebugError("Fail to open tmp file " + FormatPath(m_tmpFile));
    return false;
  }
}

// --------------------------------------------------------------------------
bool Page::OpenTempFile(std::ios_base::openmode mode) {
  assert(UseTempFile());
  if (!UseTempFile()) {
    return false;
  }

  bool success = true;
  auto file = dynamic_cast<fstream *>(m_body.get());
  if (file == nullptr) {
    success = false;
  } else {
    file->open(m_tmpFile, mode);
    if (!file->is_open()) {
      success = false;
    } /* else {
      file->flush();
    } */
  }

  DebugErrorIf(!success, "Fail to open file " + FormatPath(m_tmpFile));
  return success;
}

// --------------------------------------------------------------------------
void Page::CloseTempFile() {
  if (UseTempFile()) {
    auto file = dynamic_cast<fstream *>(m_body.get());
    if (file != nullptr) {
      file->flush();
      file->close();
    }
  }
}

// --------------------------------------------------------------------------
void Page::ResizeToSmallerSize(size_t smallerSize) {
  // Do a lazy resize:
  // 1. Change size to 'samllerSize'.
  // 2. Set output position indicator to 'samllerSize'.
  assert(0 <= smallerSize && smallerSize <= m_size);
  m_size = smallerSize;
  if (UseTempFile()) {
    m_body->seekp(m_offset + smallerSize, std::ios_base::beg);
  } else {
    m_body->seekp(smallerSize, std::ios_base::beg);
  }
}

// --------------------------------------------------------------------------
bool Page::Refresh(off_t offset, size_t len, const char *buffer,
                   const string &tmpfile) {
  if (len == 0) {
    return true;  // do nothing
  }

  bool isValidInput = offset >= m_offset && buffer != nullptr && len > 0;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to refresh page(" + ToStringLine(m_offset, m_size) +
               ") with invalid input " + ToStringLine(offset, len, buffer));
    return false;
  }
  return UnguardedRefresh(offset, len, buffer, tmpfile);
}

// --------------------------------------------------------------------------
bool Page::UnguardedRefresh(off_t offset, size_t len, const char *buffer,
                            const string &tmpfile) {
  auto moreLen = offset + len - Next();
  auto dataLen = moreLen > 0 ? m_size + moreLen : m_size;
  auto data = make_shared<IOStream>(dataLen);
  if (UseTempFile()) {
    OpenTempFile(std::ios_base::binary | std::ios_base::in);  // open for read
    m_body->seekg(offset, std::ios_base::beg);
  } else {
    m_body->seekg(0, std::ios_base::beg);
  }
  (*data) << m_body->rdbuf();
  CloseTempFile();  // must close temp file after opened
  data->seekp(offset - m_offset, std::ios_base::beg);
  data->write(buffer, len);

  if (!data->good()) {
    DebugError("Fail to refresh page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return false;
  }

  if (!tmpfile.empty()) {
    m_tmpFile = tmpfile;
    if (!SetupTempFile()) {
      DebugError("Unable to set up tmp file " + FormatPath(m_tmpFile));
      return false;
    }
  }

  if (UseTempFile()) {
    OpenTempFile(std::ios_base::binary | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
    data->seekg(0, std::ios_base::beg);
    (*m_body) << data->rdbuf();  // put pages' all content into tmp file
    CloseTempFile();
  } else {
    data->seekg(0, std::ios_base::beg);
    m_body = std::move(data);
  }
  if (moreLen > 0) {
    m_size += moreLen;
  }
  if (m_body->good()) {
    return true;
  } else {
    DebugError("Fail to refresh page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return false;
  }
}

// --------------------------------------------------------------------------
size_t Page::Read(off_t offset, size_t len, char *buffer) {
  if (len == 0) {
    return 0;  // do nothing
  }

  bool isValidInput =
      (offset >= m_offset && offset < Next() && buffer != nullptr && len > 0 &&
       len <= static_cast<size_t>(Next() - offset));
  assert(isValidInput);
  DebugErrorIf(!isValidInput,
               "Try to read page (" + ToStringLine(m_offset, m_size) +
                   ") with invalid input " + ToStringLine(offset, len, buffer));
  return UnguardedRead(offset, len, buffer);
}

// --------------------------------------------------------------------------
size_t Page::UnguardedRead(off_t offset, size_t len, char *buffer) {
  if (!m_body) {
    DebugError("null body stream " + ToStringLine(offset, len, buffer));
    return 0;
  }
  if (UseTempFile()) {
    OpenTempFile(std::ios_base::binary | std::ios_base::in);  // open for read
    m_body->seekg(offset, std::ios_base::beg);
  } else {
    m_body->seekg(offset - m_offset, std::ios_base::beg);
  }
  m_body->read(buffer, len);
  if (!m_body->good()) {
    DebugError("Fail to read page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return 0;
  } else {
    CloseTempFile();
    return len;
  }
}

// --------------------------------------------------------------------------
string ToStringLine(off_t offset, size_t len, const char *buffer) {
  return "[offset:size:buffer=" + to_string(offset) + ":" + to_string(len) +
         ":" + PointerAddress(buffer) + "]";
}

// --------------------------------------------------------------------------
string ToStringLine(off_t offset, size_t size) {
  return "[offset:size=" + to_string(offset) + ":" + to_string(size) + "]";
}

// --------------------------------------------------------------------------
string ToStringLine(const string &fileId, off_t offset, size_t len,
                    const char *buffer) {
  return "[fileId:offset:size:buffer=" + fileId + ":" + to_string(offset) +
         ":" + to_string(len) + ":" + PointerAddress(buffer) + "]";
}

}  // namespace Data
}  // namespace QS
