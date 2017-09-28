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
using QS::StringUtils::PointerAddress;
using QS::Utils::CreateDirectoryIfNotExists;
using QS::Utils::FileExists;
using QS::Utils::GetDirName;
using QS::Utils::RemoveFileIfExists;
using QS::Utils::RemoveFileIfExistsNoLog;
using std::fstream;
using std::make_shared;
using std::iostream;
using std::shared_ptr;
using std::stringstream;
using std::string;
using std::to_string;

namespace {
bool IsTempFile( const string & tmpfilePath){
  return GetDirName(tmpfilePath) == "/tmp/";
}
}  // namespace


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

  UnguardedPutToBody(offset, len, buffer);
}

// --------------------------------------------------------------------------
Page::Page(off_t offset, size_t len, const char *buffer, const string &tmpfile)
: m_offset(offset), m_size(len), m_tmpFile(tmpfile) {
  bool isValidInput = offset >= 0 && len >= 0 && buffer != NULL;
  assert(isValidInput);
  if (!isValidInput) {
    DebugError("Try to new a page with invalid input " +
               ToStringLine(offset, len, buffer));
    return;
  }

  if (SetupTempFile()){
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
Page::~Page(){
  RemoveTempFileFromDiskIfExists();
}

// --------------------------------------------------------------------------
void Page::UnguardedPutToBody(off_t offset, size_t len, const char *buffer){
  m_body->seekp(0, std::ios_base::beg);
  m_body->write(buffer, len);
  DebugErrorIf(m_body->fail(),
               "Fail to write buffer " + ToStringLine(offset, len, buffer));
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
               "Fail to write buffer " + ToStringLine(offset, len));
}

// --------------------------------------------------------------------------
void Page::SetStream(shared_ptr<iostream> &&stream) {
  m_body = std::move(stream);
}

// --------------------------------------------------------------------------
void Page::RemoveTempFileFromDiskIfExists(bool logOn) const {
  if (!m_tmpFile.empty() && FileExists(m_tmpFile, logOn)) {
    if(logOn){
      RemoveFileIfExists(m_tmpFile);
    } else {
      RemoveFileIfExistsNoLog(m_tmpFile);
    }
  }
}

// --------------------------------------------------------------------------
bool Page::SetupTempFile() {
  CreateDirectoryIfNotExists(GetCacheTemporaryDirectory());
  if (!IsTempFile(m_tmpFile)) {
    DebugError("tmp file " + m_tmpFile + " is not locating under tmp folder");
    return false;
  }

  auto file = make_shared<fstream>(m_tmpFile);
  if (file && *file) {
    SetStream(std::move(file));
    return true;
  } else {
    return false;
  }
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
bool Page::Refresh(off_t offset, size_t len, const char *buffer, const string &tmpfile) {
  bool isValidInput =
      offset >= m_offset && buffer != NULL && len > 0 && !tmpfile.empty();
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
  (*data) << m_body->rdbuf();
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
      DebugError("Unable to set up tmp file");
      return false;
    }
  }

  (*m_body) << data->rdbuf();
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
  if(!m_body->good()){
    DebugError("Fail to read page(" + ToStringLine(m_offset, m_size) +
               ") with input " + ToStringLine(offset, len, buffer));
    return 0;
  }
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
