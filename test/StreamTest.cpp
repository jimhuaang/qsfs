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

#include "gtest/gtest.h"

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/Logging.h"
#include "base/Utils.h"
#include "data/IOStream.h"
#include "data/ResourceManager.h"
#include "data/StreamBuf.h"
#include "data/StreamUtils.h"

namespace QS {

namespace Data {

using QS::Data::IOStream;
using QS::Data::StreamBuf;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;
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

void InitStreamWithNullBuffer() { StreamBuf streamBuf(Buffer(), 1); }

void InitStreamWithOverflowLength() {
  StreamBuf streamBuf(Buffer(new vector<char>(1)), 2);
}

class StreamBufTest : public Test {
 public:
  StreamBufTest() { InitLog(); }
};

TEST_F(StreamBufTest, DeathTestInitNull) {
  ASSERT_DEATH(InitStreamWithNullBuffer(), "");
}

TEST_F(StreamBufTest, DeathTestInitOverflow) {
  ASSERT_DEATH(InitStreamWithOverflowLength(), "");
}

TEST_F(StreamBufTest, Ctor) {
  auto buf = vector<char>({'0', '1', '2'});
  const StreamBuf streamBuf(Buffer(new vector<char>(buf)), buf.size());
  ASSERT_TRUE(*(streamBuf.GetBuffer()) == buf);
}

TEST_F(StreamBufTest, PrivateFunc) {
  auto buf = vector<char>({'0', '1', '2'});
  StreamBuf streamBuf(Buffer(new vector<char>(buf)), buf.size() - 1);
  EXPECT_TRUE(*(streamBuf.GetBuffer()) == buf);

  EXPECT_TRUE(*(streamBuf.begin()) == '0');
  EXPECT_TRUE(*(streamBuf.end()) == '2');

  auto buf1 = streamBuf.ReleaseBuffer();
  EXPECT_TRUE(*(buf1) == buf);
  EXPECT_FALSE(streamBuf.GetBuffer());
}

TEST(IOStreamTest, Ctor1) {
  IOStream iostream(10);
  auto buf = dynamic_cast<StreamBuf *>(iostream.rdbuf());
  EXPECT_EQ(const_cast<const StreamBuf *>(buf)->GetBuffer()->size(), 10u);
  auto buf1 = vector<char>(10);
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(buf)->GetBuffer()) == buf1);
}

TEST(IOStreamTest, Ctor2) {
  auto buf = Buffer(new vector<char>({'0', '1', '2'}));
  IOStream iostream(std::move(buf), 3);
  auto streambuf = dynamic_cast<StreamBuf *>(iostream.rdbuf());
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(streambuf)->GetBuffer()) ==
              vector<char>({'0', '1', '2'}));

  auto buf1 = Buffer(new vector<char>({'0', '1', '2'}));
  IOStream iostream1(std::move(buf1), 2);
  auto streambuf1 = dynamic_cast<StreamBuf *>(iostream1.rdbuf());
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(streambuf1)->GetBuffer()) ==
              vector<char>({'0', '1', '2'}));
}

TEST(IOStreamTest, Read1) {
  IOStream stream(Buffer(new vector<char>({'0', '1', '2'})), 3);
  stringstream ss;
  ss << stream.rdbuf();
  EXPECT_EQ(ss.str(), string("012"));

  IOStream stream1(Buffer(new vector<char>({'0', '1', '2'})), 2);
  stringstream ss1;
  ss1 << stream1.rdbuf();
  EXPECT_EQ(ss1.str(), string("01"));
}

TEST(IOStreamTest, Read2) {
  IOStream stream(Buffer(new vector<char>({'0', '1', '2'})), 3);
  stream.seekg(1, std::ios_base::beg);
  stringstream ss;
  ss << stream.rdbuf();
  EXPECT_EQ(ss.str(), string("12"));
}

TEST(IOStreamTest, Write1) {
  IOStream stream(Buffer(new vector<char>(3)), 3);
  stringstream ss("012");
  stream << ss.rdbuf();
  auto streambuf = dynamic_cast<StreamBuf *>(stream.rdbuf());
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(streambuf)->GetBuffer()) ==
              vector<char>({'0', '1', '2'}));

  IOStream stream1(Buffer(new vector<char>(2)), 2);
  stringstream ss1("012");
  stream1 << ss1.rdbuf();
  auto streambuf1 = dynamic_cast<StreamBuf *>(stream1.rdbuf());
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(streambuf1)->GetBuffer()) ==
              vector<char>({'0', '1'}));
}

TEST(IOStreamTest, Write2) {
  IOStream stream(Buffer(new vector<char>(3)), 3);
  stringstream ss("012");
  stream.seekp(1, std::ios_base::beg);
  stream << ss.rdbuf();
  auto streambuf = dynamic_cast<StreamBuf *>(stream.rdbuf());
  EXPECT_TRUE(*(const_cast<const StreamBuf *>(streambuf)->GetBuffer()) ==
              vector<char>({char(0), '0', '1'}));
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
