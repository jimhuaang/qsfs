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

#ifndef _QSFS_FUSE_INCLUDED_DATA_PAGE_H_  // NOLINT
#define _QSFS_FUSE_INCLUDED_DATA_PAGE_H_  // NOLINT

#include <stddef.h>  // for size_t

#include <sys/types.h>  // for off_t

#include <iostream>
#include <memory>
#include <set>

#include "gtest/gtest_prod.h"  // FRIEND_TEST

namespace QS {

namespace Data {

  class File;

  class Page {
  private:
   // Page attributes
   //
   // +-----------------------------------------+
   // | A File composed of two successive pages |
   // +-----------------------------------------+
   //
   // offset  stop  next   <= 1st page
   //   ^        ^  ^
   //   |________|__|________
   //   |<- size  ->|        |
   //   |___________|________|
   //   0  1  2  3  4  5  6  7
   //               ^     ^  ^
   //          offset  stop  next   <= 2nd page
   //
   // 1st Page: offset = 0, size = 4, stop = 3, next = 4
   // 2nd Page: offset = 4, size = 3, stop = 6, next = 7

   off_t m_offset = 0;     // offset from the begin of owning File
   size_t m_size = 0;      // size of bytes this page contains
   // NOTICE: body stream should be QS::Data::IOStream which associated with
   // QS::Data::StreamBuf when not use tmp file; otherwise body stream is a 
   // fstream assoicated to a tmp file.
   // With the asssoicated stream buf or tmp file, the body stream support to 
   // be read/write for multiple times, but keep in mind following things:
   // 1) always seek to the right postion before to read/write body stream;
   // 2) if use tmp file, always call OpenTempFile before read/write and call
   // CloseTempFile after read/write (or operator <<).
   std::shared_ptr<std::iostream> m_body;  // stream storing the bytes
   std::string m_tmpFile;  // tmp file is used when qsfs cache is not enough
                           // it is an absolute path of /tmp/basename

  public:
   // Construct Page from a block of bytes
   //
   // @param  : file offset, len of bytes, buffer
   // @return :
   //
   // From pointer of buffer, number of len bytes will be writen.
   // The owning file's offset is 'offset'.
   Page(off_t offset, size_t len, const char *buffer);

   // Construct Page from a block of bytes (store it in tmp file)
   //
   // @param  : file offset, len, buffer, tmp file path
   // @return : 
   Page(off_t offset, size_t len, const char *buffer,
        const std::string &tmpfile);

   // Construct Page from a stream
   //
   // @param  : file offset, len of bytes, stream
   // @return :
   //
   // From stream, number of len bytes will be writen.
   // The owning file's offset is 'offset'.
   Page(off_t offset, size_t len, const std::shared_ptr<std::iostream> &stream);

   // Construct Page from a stream (store it in tmp file)
   //
   // @param  : file offset, len of bytes, stream, tmp file
   // @return :
   Page(off_t offset, size_t len, const std::shared_ptr<std::iostream> &stream,
        const std::string &tmpfile);

   // Construct Page from a stream by moving
   //
   // @param  : file offset, file len, stream to moving
   // @return : 
   Page(off_t offset, size_t len, std::shared_ptr<std::iostream> &&body);

  public:
   Page() = delete;
   Page(Page &&) = default;
   Page(const Page &) = default;
   Page &operator=(Page &&) = default;
   Page &operator=(const Page &) = default;
   ~Page() = default;

   // Return the stop position.
   off_t Stop() const { return 0 < m_size ? m_offset + m_size - 1 : 0; }

   // Return the offset of the next successive page.
   off_t Next() const { return m_offset + m_size; }

   // Return the size
   size_t Size() const { return m_size; }

   // Return the offset
   off_t Offset() const { return m_offset; }

   // Return body
   const std::shared_ptr<std::iostream> &GetBody() const { return m_body; }
   
   // Return if page use temp file
   bool UseTempFile();

   // Refresh the page's partial content
   //
   // @param  : file offset, len of bytes to update, buffer, tmp file
   // @return : bool
   //
   // May enlarge the page's size depended on 'len', and when the len
   // is larger than page's size and using tmp file, then all page's data
   // will be put to tmp file.
   bool Refresh(off_t offset, size_t len, const char *buffer,
                const std::string &tmpfile = std::string());

   // Refresh the page's entire content with bytes from buffer,
   // without checking.
   // For internal use only.
   // Refresh the page's entire content
   //
   // @param  : buffer
   // @return : bool
   bool Refresh(const char *buffer) {
     return Refresh(m_offset, m_size, buffer);
   }

   // Read the page's content
   //
   // @param  : file offset, len of bytes to read, buffer
   // @return : size of readed bytes
   size_t Read(off_t offset, size_t len, char *buffer);

   // Read the page's partial content
   // Starting from file offset, all the page's remaining size will be read.
   size_t Read(off_t offset, char *buffer) {
     return Read(offset, Next() - offset, buffer);
   }

   // Read the page's partial content
   size_t Read(size_t len, char *buffer) {
     return Read(m_offset, len, buffer);
   }

   // Read the page's entire content to buffer.
   size_t Read(char *buffer) {
     return Read(m_offset, m_size, buffer);
   }

  private:
   // Set stream
   void SetStream(std::shared_ptr<std::iostream> && stream);

   // Setup tmp file on disk
   // - open the tmp file
   // - set stream to fstream assocating with tmp file
   bool SetupTempFile();

   // Open tmp file
   // NOTICE: need to call CloseTempFile after call OpenTempFile
   bool OpenTempFile(std::ios_base::openmode mode);

   // Close tmp file
   void CloseTempFile();

   // Do a lazy resize for page.
   void ResizeToSmallerSize(size_t smallerSize);

   // Put data to body
   // For internal use only
   void UnguardedPutToBody(off_t offset, size_t len, const char *buffer);
   void UnguardedPutToBody(off_t offset, size_t len,
                           const std::shared_ptr<std::iostream> &stream);

   // Refreseh the page's partial content without checking.
   // Starting from file offset, len of bytes will be updated.
   // For internal use only.
   bool UnguardedRefresh(off_t offset, size_t len, const char *buffer,
                         const std::string &tmpfile = std::string());

   // Refresh the page's partial content without checking.
   // Starting from file offset, all the page's remaining size will be updated.
   // For internal use only.
   bool UnguardedRefresh(off_t offset, const char *buffer) {
     return UnguardedRefresh(offset, Next() - offset, buffer);
   }

   // Refresh the page's entire content with bytes from buffer,
   // without checking.
   // For internal use only.
   bool UnguardedRefresh(const char *buffer) {
     return UnguardedRefresh(m_offset, m_size, buffer);
   }

   // Read the page's partial content without checking
   // Starting from file offset, len of bytes will be read.
   // For internal use only.
   size_t UnguardedRead(off_t offset, size_t len, char *buffer);

   // Read the page's partial content without checking
   // Starting from file offset, all the page's remaining size will be read.
   // For internal use only.
   size_t UnguardedRead(off_t offset, char *buffer) {
     return UnguardedRead(offset, Next() - offset, buffer);
   }

   // Read the page's partial content without checking
   // For internal use only.
   size_t UnguardedRead(size_t len, char *buffer) {
     return UnguardedRead(m_offset, len, buffer);
   }

   // Read the page's entire content to buffer.
   // For internal use only.
   size_t UnguardedRead(char *buffer) {
     return UnguardedRead(m_offset, m_size, buffer);
   }

   friend class File;
   FRIEND_TEST(PageTest, CtorWithTmpFile);
   FRIEND_TEST(PageTest, TestResize);
   FRIEND_TEST(PageTest, TestResizeTmpFile);
 };


 struct PageCmp {
  bool operator()(const std::shared_ptr<Page> &a,
                  const std::shared_ptr<Page> &b) const {
    return a->Offset() < b->Offset();
  }
};

using PageSet = std::set<std::shared_ptr<Page>, PageCmp>;
using PageSetConstIterator = PageSet::const_iterator;

std::string ToStringLine(const std::string &fileId, off_t offset, size_t len,
                    const char *buffer);
std::string ToStringLine(off_t offset, size_t len, const char *buffer);
std::string ToStringLine(off_t offset, size_t size);

}  // namespace Data
}  // namespace QS

#endif  // _QSFS_FUSE_INCLUDED_DATA_PAGE_H_
