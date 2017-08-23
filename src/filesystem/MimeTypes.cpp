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

#include "filesystem/MimeTypes.h"

#include <fstream>
#include <memory>
#include <mutex>  // NOLINT
#include <sstream>
#include <string>

#include "base/Exception.h"
#include "filesystem/Configure.h"

namespace QS {

namespace FileSystem {

using QS::Exception::QSException;
using std::string;
using std::unique_ptr;


static const char *CONTENT_TYPE_STREAM1 = "application/octet-stream";
//static const char *CONTENT_TYPE_STREAM2 = "binary/octet-stream";
static const char *CONTENT_TYPE_DIR = "application/x-directory";
static const char *CONTENT_TYPE_TXT = "text/plain";

static unique_ptr<MimeTypes> instance(nullptr);
static std::once_flag flag;

// --------------------------------------------------------------------------
void InitializeMimeTypes(const std::string& mimeFile) {
  std::call_once(flag, [&mimeFile] {
    instance = unique_ptr<MimeTypes>(new MimeTypes);
    instance->Initialize(mimeFile);
  });
}

// --------------------------------------------------------------------------
MimeTypes& MimeTypes::Instance() {
  std::call_once(flag, [] {
    instance = unique_ptr<MimeTypes>(new MimeTypes);
    instance->Initialize(Configure::GetMimeFile());
  });

  return *instance.get();
}

// --------------------------------------------------------------------------
string MimeTypes::Find(const string& ext) {
  auto it = m_extToMimeTypeMap.find(ext);
  return it != m_extToMimeTypeMap.end() ? it->second : string();
}

// --------------------------------------------------------------------------
void MimeTypes::Initialize(const std::string& mimeFile) {
  std::ifstream file(mimeFile);
  if(!file){
    throw QSException("Unable to open file " + mimeFile);
  }
  
  string line;
  while(getline(file, line)){
    if(line.empty()) continue;
    if(line.front() == '#') continue;

    std::stringstream ss(line);
    string mimeType;
    ss >> mimeType;
    while(ss){
      string ext;
      ss >> ext;
      if(ext.empty()) continue;
      m_extToMimeTypeMap.emplace(ext, mimeType);
    }
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

// --------------------------------------------------------------------------
string GetDirectoryMimeType() { return CONTENT_TYPE_DIR; }

// --------------------------------------------------------------------------
string GetTextMimeType() { return CONTENT_TYPE_TXT; }

}  // namespace FileSystem
}  // namespace QS
