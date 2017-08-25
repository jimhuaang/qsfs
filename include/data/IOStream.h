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

#ifndef _QSFS_FUSE_INCLUDED_DATA_IOSTREAM_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_IOSTREAM_H_  // NOLINT

#include <stddef.h>

#include <iostream>
#include <string>  // for std::char_traits

namespace QS {

namespace Data {

/**
 * Encapsulates and manages ownership of custom iostream.
 * This is a move only type.
 */
class IOStream : public std::basic_iostream<char, std::char_traits<char> > {
  using Base = std::basic_iostream<char, std::char_traits<char> >;

 public:
  explicit IOStream(size_t bufSize);
  IOStream(IOStream &&) = default;
  IOStream(const IOStream &) = delete;
  IOStream &operator=(IOStream &&) = default;
  IOStream &operator=(const IOStream &) = delete;
  virtual ~IOStream();

 private:
  IOStream() = default;
};

}  // namespace Data
}  // namespace QS

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDED_DATA_IOSTREAM_H_
