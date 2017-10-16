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

#ifndef _QSFS_FUSE_INCLUDED_DATA_STREAMBUF_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_STREAMBUF_H_  // NOLINT

#include <streambuf>  // NOLINT

#include <stddef.h>  // for size_t

#include "gtest/gtest_prod.h"  // FRIEND_TEST

#include <memory>
#include <vector>

namespace QS {

namespace Client {
class QSTransferManager;
}  // namespace Client

namespace Data {

class IOSTream;

using Buffer = std::unique_ptr<std::vector<char> >;

/**
 * An exclusive ownership stream buf to use with std::iostream
 * that uses a preallocated buffer under the hood.
 */
class StreamBuf : public std::streambuf {
 public:
  StreamBuf(Buffer buf, size_t lenghtToRead);

  StreamBuf() = delete;
  StreamBuf(StreamBuf &&) = default;
  StreamBuf(const StreamBuf &) = delete;
  StreamBuf &operator=(StreamBuf &&) = default;
  StreamBuf &operator=(const StreamBuf &) = delete;
  ~StreamBuf() = default;

 public:
  const Buffer &GetBuffer() const { return m_buffer; }

 protected:
  pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) override;
  pos_type seekpos(pos_type pos,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) override;

 private:
  Buffer &GetBuffer() { return m_buffer; }
  // Release buffer ownership
  Buffer ReleaseBuffer();

  char *begin() { return &(*m_buffer)[0]; }
  char *end() { return begin() + m_lengthToRead; }

 private:
  Buffer m_buffer;
  size_t m_lengthToRead;  // length in bytes to actually use in the buffer
                          // e.g. you have a 1kb buffer, but only want
                          // stream to see 500 b of it.
  friend class IOStream;
  friend class QS::Client::QSTransferManager;

  FRIEND_TEST(StreamBufTest, PrivateFunc);
};

}  // namespace Data
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDED_DATA_STREAMBUF_H_
