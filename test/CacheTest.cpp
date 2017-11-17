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

#include <string.h>

#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "base/Logging.h"
#include "base/Utils.h"
#include "data/Cache.h"

namespace QS {

namespace Data {

using std::make_shared;
using std::stringstream;
using std::vector;
using std::unique_ptr;
using ::testing::Test;

// default log dir
static const char *defaultLogDir = "/tmp/qsfs.logs/";
void InitLog() {
  QS::Utils::CreateDirectoryIfNotExistsNoLog(defaultLogDir);
  QS::Logging::InitializeLogging(
      unique_ptr<QS::Logging::Log>(new QS::Logging::DefaultLog(defaultLogDir)));
  EXPECT_TRUE(QS::Logging::GetLogInstance() != nullptr)
      << "log instance is null";
}

uid_t uid_ = 1000U;
gid_t gid_ = 1000U;
mode_t fileMode_ = S_IRWXU | S_IRWXG | S_IROTH;

class CacheTest : public Test {
 protected:
  static void SetUpTestCase() { InitLog(); }
};

// --------------------------------------------------------------------------
TEST_F(CacheTest, Default) {
  uint64_t cacheCap = 100;
  Cache cache(cacheCap);
  EXPECT_TRUE(cache.HasFreeSpace(cacheCap));
  EXPECT_FALSE(cache.HasFreeSpace(cacheCap + 1));
  EXPECT_EQ(cache.GetSize(), 0u);
  EXPECT_EQ(cache.GetCapacity(), cacheCap);
  EXPECT_EQ(cache.GetNumFile(), 0u);
  EXPECT_TRUE(cache.Begin() == cache.End());
  EXPECT_TRUE(cache.Free(cacheCap, ""));
  EXPECT_FALSE(cache.IsLastFileOpen());
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Write) {
  uint64_t cacheCap = 100;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);
  EXPECT_FALSE(cache.HasFreeSpace(cacheCap));
  EXPECT_TRUE(cache.HasFreeSpace(cacheCap - len1));
  EXPECT_EQ(cache.GetSize(), len1);
  EXPECT_EQ(cache.GetCapacity(), cacheCap);
  EXPECT_EQ(cache.GetNumFile(), 1u);
  EXPECT_TRUE(cache.Begin() != cache.End());
  EXPECT_EQ(cache.Find("file1"), cache.Begin());
  EXPECT_TRUE(cache.HasFile("file1"));
  EXPECT_TRUE(cache.HasFileData("file1", off1, len1));
  EXPECT_FALSE(cache.HasFileData("file1", off1, len1 + 1));
  EXPECT_FALSE(cache.HasFileData("file1", off1 + 1, len1));
  EXPECT_TRUE(cache.GetUnloadedRanges("file1", 0, len1).empty());
  EXPECT_FALSE(cache.GetUnloadedRanges("file1", 0, len1 + 1).empty());
  ContentRangeDeque range1 = {{len1, 1}};
  EXPECT_EQ(cache.GetUnloadedRanges("file1", 0, len1 + 1), range1);

  EXPECT_EQ(cache.GetTime("file1"), (time_t)0);
  time_t newtime = time(NULL);
  cache.SetTime("file1", newtime);
  EXPECT_EQ(cache.GetTime("file1"), newtime);
  EXPECT_FALSE(cache.IsLastFileOpen());
  cache.SetFileOpen("file1", true);
  EXPECT_TRUE(cache.IsLastFileOpen());
  cache.SetFileOpen("file1", false);

  size_t newSize = 2;
  cache.Resize("file1", newSize, newtime);
  EXPECT_EQ(cache.GetSize(), newSize);

  cache.Rename("file1", "newfile1");
  EXPECT_FALSE(cache.HasFile("file1"));
  EXPECT_TRUE(cache.HasFile("newfile1"));

  EXPECT_FALSE(cache.Free(cacheCap, "newfile1"));
  EXPECT_TRUE(cache.Free(cacheCap, ""));
  EXPECT_FALSE(cache.HasFile("newfile1"));

  EXPECT_TRUE(cache.HasFreeSpace(cacheCap));
  cache.Write("file1", off1, len1, page1, 0);  // write again after free
  EXPECT_TRUE(cache.HasFile("file1"));
  EXPECT_EQ(cache.Erase("file1"), cache.End());
  EXPECT_FALSE(cache.HasFile("file1"));
  EXPECT_TRUE(cache.HasFreeSpace(cacheCap));
  cache.Write("file1", off1, len1, page1, 0);  // write again after erase
  EXPECT_TRUE(cache.HasFile("file1"));

  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  EXPECT_EQ(cache.GetNumFile(), 1u);
  EXPECT_EQ(cache.GetSize(), len1 + len2);
  EXPECT_TRUE(cache.GetUnloadedRanges("file1", 0, len1 + len2).empty());
  ContentRangeDeque range2 = {{len1 + len2, 1}};
  EXPECT_EQ(cache.GetUnloadedRanges("file1", 0, len1 + len2 + 1), range2);

  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);
  EXPECT_EQ(cache.GetSize(), len1 + len2 + len3);
  EXPECT_TRUE(cache.HasFileData("file1", off1, len1 + len2));
  EXPECT_FALSE(cache.HasFileData("file1", off1, len1 + len2 + 1));
  EXPECT_FALSE(cache.HasFileData("file1", off2 + len2, 1));
  ContentRangeDeque range3 = {{len1 + len2, holeLen}};
  EXPECT_EQ(cache.GetUnloadedRanges("file1", 0, len1 + len2 + holeLen + len3),
            range3);

  cache.Write("file2", off1, len1, page1, 0);
  EXPECT_EQ(cache.GetNumFile(), 2u);
  EXPECT_EQ(cache.GetSize(), 2 * len1 + len2 + len3);
  EXPECT_EQ(cache.Find("file2"), cache.Begin());
  EXPECT_TRUE(cache.HasFile("file1"));
  EXPECT_EQ(cache.Find("file1"), ++cache.Begin());
  EXPECT_TRUE(cache.Free(cacheCap - len1, "file2"));
  EXPECT_FALSE(cache.HasFile("file1"));
  EXPECT_EQ(cache.GetSize(), len1);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, WriteDiskFile) {
  uint64_t cacheCap = 3;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);

  auto pfile = cache.Find("file1");
  EXPECT_EQ(pfile, cache.Begin());

  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);

  EXPECT_EQ(cache.GetFileSize("file1"), len1 + len2 + len3);
  pfile = cache.Find("file1");
  EXPECT_TRUE(pfile->second->UseDiskFile());

  EXPECT_EQ(cache.GetSize(), len1);
  EXPECT_TRUE(cache.HasFileData("file1", off1, len1 + len2));
  EXPECT_FALSE(cache.HasFileData("file1", off1, len1 + len2 + 1));
  EXPECT_FALSE(cache.HasFileData("file1", off2 + len2, 1));
  ContentRangeDeque range3 = {{len1 + len2, holeLen}};
  EXPECT_EQ(cache.GetUnloadedRanges("file1", 0, len1 + len2 + holeLen + len3),
            range3);

  cache.Write("file2", off1, len1, page1, 0);
  EXPECT_FALSE(cache.HasFile("file1"));
  EXPECT_EQ(cache.GetNumFile(), 1u);
  EXPECT_EQ(cache.GetSize(), len1);
  EXPECT_EQ(cache.Find("file2"), cache.Begin());
  EXPECT_TRUE(cache.Free(cacheCap, ""));
  EXPECT_FALSE(cache.HasFile("file2"));
  EXPECT_EQ(cache.GetSize(), 0u);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Resize) {
  uint64_t cacheCap = 100;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);
  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);
  cache.Write("file2", off1, len1, page1, 0);

  EXPECT_EQ(cache.GetFileSize("file1"), len1 + len2 + len3);
  EXPECT_EQ(cache.GetFileSize("file2"), len1);

  auto newFile1Sz = len1 + len2 + 1;
  cache.Resize("file1", newFile1Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file1"), newFile1Sz);
  auto newFile2Sz = len1 - 1;
  cache.Resize("file2", newFile2Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file2"), newFile2Sz);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, ResizeDiskFile) {
  uint64_t cacheCap = 3;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);
  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);
  EXPECT_EQ(cache.GetFileSize("file1"), len1 + len2 + len3);
  auto newFile1Sz = len1 + len2 + 1;
  cache.Resize("file1", newFile1Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file1"), newFile1Sz);

  cache.Write("file2", off1, len1, page1, 0);
  EXPECT_FALSE(cache.HasFile("file1"));
  EXPECT_EQ(cache.GetFileSize("file2"), len1);
  auto newFile2Sz = len1 - 1;
  cache.Resize("file2", newFile2Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file2"), newFile2Sz);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Read) {
  uint64_t cacheCap = 100;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);
  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);
  cache.Write("file2", off1, len1, page1, 0);

  EXPECT_EQ(cache.GetFileSize("file1"), len1 + len2 + len3);
  EXPECT_EQ(cache.GetFileSize("file2"), len1);

  auto newFile1Sz = len1 + len2 + 1;
  cache.Resize("file1", newFile1Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file1"), newFile1Sz);
  vector<char> buf1(newFile1Sz);
  cache.Read("file1", 0, newFile1Sz, &buf1[0]);
  vector<char> arr1{'0', '1', '2', 'a', 'b', 'c', '\0'};
  EXPECT_EQ(buf1, arr1);

  vector<char> buf2(newFile1Sz + holeLen);
  cache.Read("file1", 0, newFile1Sz + holeLen, &buf2[0]);
  vector<char> arr2{'0', '1', '2', 'a', 'b', 'c'};
  for (size_t i = 0; i < holeLen; ++i) {
    arr2.push_back('\0');
  }
  arr2.push_back('A');
  EXPECT_EQ(buf2, arr2);

  vector<char> buf3(len2 + holeLen + 1);
  cache.Read("file1", off2, len2 + holeLen + 1, &buf3[0]);
  vector<char> arr3{'a', 'b', 'c'};
  for (size_t i = 0; i < holeLen; ++i) {
    arr3.push_back('\0');
  }
  arr3.push_back('A');
  EXPECT_EQ(buf3, arr3);

  auto newFile1Sz_ = len1 + len2 + len3;
  cache.Resize("file1", newFile1Sz_, 0);  // resize to larger
  EXPECT_EQ(cache.GetFileSize("file1"), newFile1Sz_);
  vector<char> buf1_(newFile1Sz_);
  cache.Read("file1", 0, newFile1Sz_, &buf1_[0]);
  vector<char> arr1_{'0', '1', '2', 'a', 'b', 'c', '\0', '\0', '\0'};
  EXPECT_EQ(buf1_, arr1_);
  vector<char> buf2_(newFile1Sz_ + holeLen);
  cache.Read("file1", 0, newFile1Sz_ + holeLen, &buf2_[0]);
  vector<char> arr2_{'0', '1', '2', 'a', 'b', 'c'};
  for (size_t i = 0; i < holeLen; ++i) {
    arr2_.push_back('\0');
  }
  arr2_.push_back('A');
  arr2_.push_back('\0');
  arr2_.push_back('\0');
  EXPECT_EQ(buf2_, arr2_);

  auto newFile2Sz = len1 - 1;
  cache.Resize("file2", newFile2Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file2"), newFile2Sz);
  vector<char> buf4(newFile2Sz);
  cache.Read("file2", 0, newFile2Sz, &buf4[0]);
  vector<char> arr4{'0', '1'};
  EXPECT_EQ(buf4, arr4);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, ReadDiskFile) {
  uint64_t cacheCap = 3;
  Cache cache(cacheCap);

  constexpr const char *page1 = "012";
  constexpr size_t len1 = strlen(page1);
  off_t off1 = 0;
  cache.Write("file1", off1, len1, page1, 0);
  cache.SetFileOpen("file1", true);

  constexpr const char *data = "abc";
  constexpr size_t len2 = strlen(data);
  off_t off2 = off_t(len1);
  auto page2 = make_shared<stringstream>(data);
  cache.Write("file1", off2, len2, std::move(page2), 0);
  constexpr const char *page3 = "ABC";
  constexpr size_t len3 = strlen(page3);
  constexpr size_t holeLen = 10;
  off_t off3 = off2 + holeLen + len3;
  cache.Write("file1", off3, len3, page3, 0);
  cache.Write("file2", off1, len1, page1, 0);

  EXPECT_EQ(cache.GetFileSize("file1"), len1 + len2 + len3);
  EXPECT_EQ(cache.GetFileSize("file2"), len1);

  auto newFile1Sz = len1 + len2 + 1;
  cache.Resize("file1", newFile1Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file1"), newFile1Sz);
  vector<char> buf1(newFile1Sz);
  cache.Read("file1", 0, newFile1Sz, &buf1[0]);
  vector<char> arr1{'0', '1', '2', 'a', 'b', 'c', '\0'};
  EXPECT_EQ(buf1, arr1);

  vector<char> buf2(newFile1Sz + holeLen);
  cache.Read("file1", 0, newFile1Sz + holeLen, &buf2[0]);
  vector<char> arr2{'0', '1', '2', 'a', 'b', 'c'};
  for (size_t i = 0; i < holeLen; ++i) {
    arr2.push_back('\0');
  }
  arr2.push_back('A');
  EXPECT_EQ(buf2, arr2);

  vector<char> buf3(len2 + holeLen + 1);
  cache.Read("file1", off2, len2 + holeLen + 1, &buf3[0]);
  vector<char> arr3{'a', 'b', 'c'};
  for (size_t i = 0; i < holeLen; ++i) {
    arr3.push_back('\0');
  }
  arr3.push_back('A');
  EXPECT_EQ(buf3, arr3);

  auto newFile2Sz = len1 - 1;
  cache.Resize("file2", newFile2Sz, 0);
  EXPECT_EQ(cache.GetFileSize("file2"), newFile2Sz);
  vector<char> buf4(newFile2Sz);
  cache.Read("file2", 0, newFile2Sz, &buf4[0]);
  vector<char> arr4{'0', '1'};
  EXPECT_EQ(buf4, arr4);
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
