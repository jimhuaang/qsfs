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

#ifndef _QSFS_FUSE_INCLUDED_FILESYSTEM_MIMETYPES_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_FILESYSTEM_MIMETYPES_H_  // NOLINT

#include <string.h>  // for strcasecmp

#include <map>

namespace QS {

namespace FileSystem {

void InitializeMimeTypes(const std::string &mimeFile);

class MimeTypes {
 public:
  MimeTypes(MimeTypes &&) = delete;
  MimeTypes(const MimeTypes &) = delete;
  MimeTypes &operator=(MimeTypes &&) = delete;
  MimeTypes &operator=(const MimeTypes &) = delete;
  ~MimeTypes() = default;

 public:
  static MimeTypes &Instance();

  // Find Mime Type by extension
  //
  // @param  : ext
  // @return : mime type, or empty string if not found
  std::string Find(const std::string &ext);

 private:
  MimeTypes() = default;
  void Initialize(const std::string &mimeFile);

  struct CaseInsensitiveCmp {
    bool operator()(const std::string &lhs, const std::string &rhs) const {
      return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
  };

  // mime type to extension map
  std::map<std::string, std::string, CaseInsensitiveCmp> m_extToMimeTypeMap;

  friend void InitializeMimeTypes(const std::string &mimeFile);
};

// Look up the mime type from the file path
//
// @param  : e.g., "index.html"
// @return : e.g., "text/html"
std::string LookupMimeType(const std::string &path);

// Get mime type for directory
//
// @param  : void
// @return : dir mime type
std::string GetDirectoryMimeType();

// Get mime type for text file
//
// @param  : void
// @return : text mime type
std::string GetTextMimeType();

}  // namespace FileSystem
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_FILESYSTEM_MIMETYPES_H_
