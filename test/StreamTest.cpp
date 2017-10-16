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
#include <vector>

#include "base/Logging.h"
#include "base/Utils.h"
#include "data/IOStream.h"
#include "data/ResourceManager.h"
#include "data/StreamBuf.h"
#include "data/StreamUtils.h"


namespace QS {

namespace Data {

using QS::Data::StreamBuf;
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

void InitStreamWithNullBuffer() {
  StreamBuf streamBuf(Buffer(), 1);
}

void InitStreamWithOverflowLength() {
  StreamBuf streamBuf(Buffer(new vector<char>(1)), 2);
}

class StreamBufTest : public Test {
 public:
  StreamBufTest() { InitLog(); }

/*  protected:
  StreamBuf *m_pStreamBuf = nullptr; */
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
  ASSERT_TRUE(*(streamBuf.GetBuffer()) == buf);

  ASSERT_TRUE(*(streamBuf.begin()) == '0');
  ASSERT_TRUE(*(streamBuf.end()) == '2');

  auto buf1 = streamBuf.ReleaseBuffer();
  ASSERT_TRUE(*(buf1) == buf);
  ASSERT_FALSE(streamBuf.GetBuffer());
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
