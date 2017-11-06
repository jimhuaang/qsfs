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
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>
#include <utility>

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "configure/Default.h"
#include "data/IOStream.h"
#include "data/StreamUtils.h"

namespace QS {

namespace Data {

using QS::Data::IOStream;
using QS::Data::StreamUtils::GetStreamSize;
using QS::Configure::Default::GetCacheTemporaryDirectory;
using QS::StringUtils::FormatPath;
using QS::StringUtils::PointerAddress;
using QS::Utils::CreateDirectoryIfNotExists;
using QS::Utils::FileExists;
using QS::Utils::GetDirName;
using std::fstream;
using std::lock_guard;
using std::make_shared;
using std::iostream;
using std::recursive_mutex;
using std::shared_ptr;
using std::stringstream;
using std::string;
using std::to_string;

namespace {
bool IsTempFile(const string &tmpfilePath) {
  return GetDirName(tmpfilePath) == GetCacheTemporaryDirectory();
}

// RAII class to open file and close it in destructor
class FileOpener {
 public:
  explicit FileOpener(const shared_ptr<iostream> &body) {
    m_file = dynamic_cast<fstream *>(body.get());
  }

  FileOpener(const FileOpener &) = delete;
  FileOpener(FileOpener &&) = delete;
  FileOpener& operator=(const FileOpener &) = delete;
  FileOpener& operator=(FileOpener &&) = delete;
  
  ~FileOpener() {
    if(m_file != nullptr && m_file->is_open()) {
      m_file->flush();
      m_file->close();
    }
  }

  void DoOpen(const string &filename, std::ios_base::openmode mode) {
    if(m_file != nullptr && !filename.empty()) {
      m_file->open(filename, mode);
      if (!m_file->is_open()) {
        DebugError("Fail to open file " + FormatPath(filename));
      }
    } else {
      DebugError("Invalid parameter");
    }
  }
private:
  FileOpener() = default;

  fstream *m_file = nullptr;
};

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

  lock_guard<recursive_mutex> lock(m_mutex);
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

  lock_guard<recursive_mutex> lock(m_mutex);
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

  lock_guard<recursive_mutex> lock(m_mutex);
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

  lock_guard<recursive_mutex> lock(m_mutex);
  if (SetupTempFile()) {
    UnguardedPutToBody(offset, len, instream);
  }
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, shared_ptr<iostream> &&body)
    : m_offset(offset), m_size(len) {
  size_t streamlen = GetStreamSize(body);
  if (streamlen < len) {
    DebugWarning(
        "Input stream buffer len is less than parameter 'len', Ajust it");
    m_size = streamlen;
  }

  lock_guard<recursive_mutex> lock(m_mutex);
  // To make sure used QS::Data::IOStream to store body, as we need to
  // support multipule read/write ops.
  auto instream = std::move(body);
  IOStream *pInstream = dynamic_cast<IOStream *>(instream.get());
  if (pInstream != nullptr) {
    m_body = std::move(instream);
    pInstream = nullptr;
  } else {
    m_body = make_shared<IOStream>(m_size);
    UnguardedPutToBody(m_offset, m_size, instream);
  }
}

// --------------------------------------------------------------------------
bool Page::UseTempFile() {
  lock_guard<recursive_mutex> lock(m_mutex);
  return !m_tmpFile.empty(); 
}

// --------------------------------------------------------------------------
bool Page::UseTempFileNoLock() {
  return !m_tmpFile.empty(); 
}

// --------------------------------------------------------------------------
void Page::UnguardedPutToBody(off_t offset, size_t len, const char *buffer) {
  if (!m_body) {
    DebugError("null body stream " + ToStringLine(offset, len, buffer));
    return;
  }
  FileOpener opener(m_body);
  if (UseTempFileNoLock()) {
    // Notice: need to open in both output and input mode to avoid truncate file
    // as open in out mode only will actually truncate file.
    opener.DoOpen(m_tmpFile, std::ios_base::binary | std::ios_base::ate |
                  std::ios_base::in | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
  } else {
    m_body->seekp(0, std::ios_base::beg);
  }
  if(!m_body->good()) {
    DebugError("Fail to seek body " + ToStringLine(offset, len, buffer));
  } else {
    m_body->write(buffer, len);
    if(!m_body->good()) {
      DebugError("Fail to write buffer " + ToStringLine(offset, len, buffer));
    }
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

  FileOpener opener(m_body);
  if (UseTempFileNoLock()) {
    opener.DoOpen(m_tmpFile, std::ios_base::binary | std::ios_base::ate |
                  std::ios_base::in | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
  } else {
    m_body->seekp(0, std::ios_base::beg);
  }
  if(!m_body->good()) {
    DebugError("Fail to seek body " + ToStringLine(offset, len));
  } else {
    instream->seekg(0, std::ios_base::beg);
    if (len == instreamLen) {
      (*m_body) << instream->rdbuf();
    } else if (len < instreamLen) {
      stringstream ss;
      ss << instream->rdbuf();
      m_body->write(ss.str().c_str(), len);
    }
    if(!m_body->good()) {
      DebugError("Fail to write buffer " + ToStringLine(offset, len));
    }
  }
}

// --------------------------------------------------------------------------
void Page::SetStream(shared_ptr<iostream> &&stream) {
  lock_guard<recursive_mutex> lock(m_mutex);
  m_body = std::move(stream);
}

// --------------------------------------------------------------------------
bool Page::SetupTempFile() {
  lock_guard<recursive_mutex> lock(m_mutex);
  CreateDirectoryIfNotExists(GetCacheTemporaryDirectory());
  if (!IsTempFile(m_tmpFile)) {
    DebugError("tmp file not under /tmp " + FormatPath(m_tmpFile));
    return false;
  }

  std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::ate |
                                 std::ios_base::in | std::ios_base::out;
  if (!FileExists(m_tmpFile)) {
    // Notice: set open mode with std::ios_base::out to create file in disk if
    // it is not exist
    mode = std::ios_base::binary | std::ios_base::out;
  }
  auto file = make_shared<fstream>(m_tmpFile, mode);
  if (file && file->is_open()) {
    DebugInfo("Open tmp file " + FormatPath(m_tmpFile));
  }
  file->close();
  m_body = std::move(file);
  return true;
}

// --------------------------------------------------------------------------
void Page::ResizeToSmallerSize(size_t smallerSize) {
  // Do a lazy resize:
  // 1. Change size to 'samllerSize'.
  // 2. Set output position indicator to 'samllerSize'.
  assert(0 <= smallerSize && smallerSize <= m_size);
  lock_guard<recursive_mutex> lock(m_mutex);
  m_size = smallerSize;
  if (UseTempFileNoLock()) {
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

  lock_guard<recursive_mutex> lock(m_mutex);
  return UnguardedRefresh(offset, len, buffer, tmpfile);
}

// --------------------------------------------------------------------------
bool Page::UnguardedRefresh(off_t offset, size_t len, const char *buffer,
                            const string &tmpfile) {
  auto moreLen = offset + len - Next();
  auto dataLen = moreLen > 0 ? m_size + moreLen : m_size;
  auto data = make_shared<IOStream>(dataLen);
  {
    FileOpener opener(m_body);
    if (UseTempFileNoLock()) {
      opener.DoOpen(m_tmpFile,
                    std::ios_base::binary | std::ios_base::in);  // open for read
      m_body->seekg(offset, std::ios_base::beg);
    } else {
      m_body->seekg(0, std::ios_base::beg);
    }
    (*data) << m_body->rdbuf();
  }

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

  FileOpener opener(m_body);
  if (UseTempFileNoLock()) {
    opener.DoOpen(m_tmpFile, std::ios_base::binary | std::ios_base::ate |
                  std::ios_base::in | std::ios_base::out);  // open for write
    m_body->seekp(m_offset, std::ios_base::beg);
    if(!m_body->good()) {
      DebugError("Fail to seek page(" + ToStringLine(m_offset, m_size) +
                 ") with input " + ToStringLine(offset, len, buffer));
      return false;
    }
    data->seekg(0, std::ios_base::beg);
    (*m_body) << data->rdbuf();  // put pages' all content into tmp file
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
  
  lock_guard<recursive_mutex> lock(m_mutex);
  return UnguardedRead(offset, len, buffer);
}

// --------------------------------------------------------------------------
size_t Page::UnguardedRead(off_t offset, size_t len, char *buffer) {
  if (!m_body) {
    DebugError("null body stream " + ToStringLine(offset, len, buffer));
    return 0;
  }
  FileOpener opener(m_body);
  if (UseTempFileNoLock()) {
    opener.DoOpen(m_tmpFile,
                  std::ios_base::binary | std::ios_base::in);  // open for read
    m_body->seekg(offset, std::ios_base::beg);
  } else {
    m_body->seekg(offset - m_offset, std::ios_base::beg);
  }
  if (!m_body->good()) {
    DebugError("Fail to seek page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return 0;
  }

  m_body->read(buffer, len);
  if (!m_body->good()) {
    DebugError("Fail to read page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return 0;
  } else {
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
