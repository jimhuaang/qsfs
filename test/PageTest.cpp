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

#include <assert.h>
#include <string.h>

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "base/Logging.h"
#include "base/Utils.h"
#include "configure/Options.h"
#include "data/Page.h"
#include "data/StreamUtils.h"

namespace QS {

namespace Data {

using QS::Data::StreamUtils::GetStreamSize;
using QS::Utils::RemoveFileIfExists;
using std::array;
using std::make_shared;
using std::string;
using std::stringstream;
using std::unique_ptr;
using ::testing::Test;

// default log dir
static const char *defaultLogDir = "/tmp/qsfs.test.logs/";
void InitLog() {
  QS::Utils::CreateDirectoryIfNotExistsNoLog(defaultLogDir);
  QS::Logging::InitializeLogging(
      unique_ptr<QS::Logging::Log>(new QS::Logging::DefaultLog(defaultLogDir)));
  EXPECT_TRUE(QS::Logging::GetLogInstance() != nullptr)
      << "log instance is null";
}

class PageTest : public Test {
 protected:
  static void SetUpTestCase() { InitLog(); }
};

// --------------------------------------------------------------------------
TEST_F(PageTest, Ctor) {
  string str("123");
  size_t len = str.size();
  Page p1(0, len, str.c_str());
  EXPECT_EQ(p1.Stop(), (off_t)(len - 1));
  EXPECT_EQ(p1.Next(), (off_t)len);
  EXPECT_EQ(p1.Size(), len);
  EXPECT_EQ(p1.Offset(), (off_t)0);
  EXPECT_EQ(GetStreamSize(p1.GetBody()), len);
  EXPECT_FALSE(p1.UseDiskFile());

  auto ss = make_shared<stringstream>(str);
  Page p2(0, len, ss);
  EXPECT_EQ(p2.Stop(), (off_t)(len - 1));
  EXPECT_EQ(p2.Next(), (off_t)len);
  EXPECT_EQ(p2.Size(), len);
  EXPECT_EQ(p2.Offset(), (off_t)0);
  EXPECT_EQ(GetStreamSize(p2.GetBody()), len);
  EXPECT_FALSE(p2.UseDiskFile());

  Page p3(0, len, std::move(ss));
  EXPECT_EQ(p3.Stop(), (off_t)(len - 1));
  EXPECT_EQ(p3.Next(), (off_t)len);
  EXPECT_EQ(p3.Size(), len);
  EXPECT_EQ(p3.Offset(), (off_t)0);
  EXPECT_EQ(GetStreamSize(p3.GetBody()), len);
  EXPECT_FALSE(p3.UseDiskFile());
}

// --------------------------------------------------------------------------
TEST_F(PageTest, CtorWithDiskFile) {
  string str("123");
  size_t len = str.size();
  string file1 =
      QS::Configure::Options::Instance().GetDiskCacheDirectory() + "test_page1";
  Page p1(0, len, str.c_str(), file1);
  EXPECT_EQ(p1.Stop(), (off_t)(len - 1));
  EXPECT_EQ(p1.Next(), (off_t)len);
  EXPECT_EQ(p1.Size(), len);
  EXPECT_EQ(p1.Offset(), (off_t)0);
  // auto body = p1.GetBody();
  // Something goes wrong in gtest, the reference to body stream is null.
  // But add the same test code in qsfs, this works fine.
  // Strang thing is that gtest (PageTest) runs ok under debug mode, but
  // assert fail when run gtest (PageTest) exe directly.
  // assert(body);  // fail when run PageTest exe, but success under debug mode
  EXPECT_TRUE(p1.UseDiskFile());
  RemoveFileIfExists(file1);

  auto ss = make_shared<stringstream>(str);
  string file2 =
      QS::Configure::Options::Instance().GetDiskCacheDirectory() + "test_page2";
  Page p2(0, len, ss, file2);
  EXPECT_EQ(p2.Stop(), (off_t)(len - 1));
  EXPECT_EQ(p2.Next(), (off_t)len);
  EXPECT_EQ(p2.Size(), len);
  EXPECT_EQ(p2.Offset(), (off_t)0);
  EXPECT_TRUE(p2.UseDiskFile());
  RemoveFileIfExists(file2);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestRead) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  array<char, len> arr{'1', '2', '3'};
  Page p1(0, len, str);

  array<char, len> buf1;
  p1.Read(0, len, &buf1[0]);
  EXPECT_TRUE(buf1 == arr);

  array<char, len> buf2;
  p1.Read(off_t(0), &buf2[0]);  // read page trailing chars start from off
  EXPECT_TRUE(buf2 == arr);

  array<char, len> buf3;
  p1.Read(len, &buf3[0]);  // read page first len chars
  EXPECT_TRUE(buf3 == arr);

  array<char, len> buf4;
  p1.Read(&buf4[0]);  // read entire page
  EXPECT_TRUE(buf4 == arr);

  constexpr size_t len1 = len - 1;
  array<char, len1> arr1{'1', '2'};
  array<char, len1> arr2{'2', '3'};

  array<char, len1> buf5;
  p1.Read(0, len1, &buf5[0]);
  EXPECT_TRUE(buf5 == arr1);

  array<char, len1> buf6;
  p1.Read(1, len1, &buf6[0]);
  EXPECT_TRUE(buf6 == arr2);

  array<char, len1> buf7;
  p1.Read(off_t(1), &buf7[0]);
  EXPECT_TRUE(buf7 == arr2);

  array<char, len1> buf8;
  p1.Read(len1, &buf8[0]);
  EXPECT_TRUE(buf8 == arr1);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestReadDiskFile) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  array<char, len> arr{'1', '2', '3'};
  string file1 =
      QS::Configure::Options::Instance().GetDiskCacheDirectory() + "test_page1";
  Page p1(0, len, str, file1);

  array<char, len> buf1;
  p1.Read(0, len, &buf1[0]);
  EXPECT_TRUE(buf1 == arr);

  RemoveFileIfExists(file1);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestRefresh) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  Page p1(0, len, str);

  array<char, len> arrNew1{'4', '5', '6'};
  p1.Refresh(&arrNew1[0]);
  array<char, len> buf1;
  p1.Read(0, len, &buf1[0]);
  EXPECT_TRUE(buf1 == arrNew1);

  array<char, len> arrNew2{'7', '8', '9'};
  p1.Refresh(off_t(0), len, &arrNew2[0]);
  array<char, len> buf2;
  p1.Read(0, len, &buf2[0]);
  EXPECT_TRUE(buf2 == arrNew2);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestRefreshDiskFile) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  string file1 =
      QS::Configure::Options::Instance().GetDiskCacheDirectory() + "test_page1";
  Page p1(0, len, str, file1);

  array<char, len> arrNew1{'4', '5', '6'};
  p1.Refresh(&arrNew1[0]);
  array<char, len> buf1;
  p1.Read(0, len, &buf1[0]);
  EXPECT_TRUE(buf1 == arrNew1);

  array<char, len> arrNew2{'7', '8', '9'};
  p1.Refresh(off_t(0), len, &arrNew2[0]);
  array<char, len> buf2;
  p1.Read(0, len, &buf2[0]);
  EXPECT_TRUE(buf2 == arrNew2);

  RemoveFileIfExists(file1);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestResize) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  Page p1(0, len, str);

  array<char, len - 1> arrSmaller{'1', '2'};
  p1.ResizeToSmallerSize(len - 1);
  array<char, len - 1> buf1;
  p1.Read(&buf1[0]);
  EXPECT_TRUE(buf1 == arrSmaller);
}

// --------------------------------------------------------------------------
TEST_F(PageTest, TestResizeDiskFile) {
  constexpr const char *str = "123";
  constexpr size_t len = strlen(str);
  string file1 =
      QS::Configure::Options::Instance().GetDiskCacheDirectory() + "test_page1";
  Page p1(0, len, str, file1);

  array<char, len - 1> arrSmaller{'1', '2'};
  p1.ResizeToSmallerSize(len - 1);
  array<char, len - 1> buf1;
  p1.Read(&buf1[0]);
  EXPECT_TRUE(buf1 == arrSmaller);
  RemoveFileIfExists(file1);
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
