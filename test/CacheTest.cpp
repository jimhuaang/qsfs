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

#include <memory>

#include "base/Logging.h"
#include "base/Utils.h"
#include "data/Cache.h"

namespace QS {

namespace Data {

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

class CacheTest : public Test {
protected:
  static void SetUpTestCase() { InitLog(); }
};

// --------------------------------------------------------------------------
TEST_F(CacheTest, Default) {
  Cache cache(100);
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Read) {

}

// --------------------------------------------------------------------------
TEST_F(CacheTest, ReadTmpFile) {

}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Write) {

}

// --------------------------------------------------------------------------
TEST_F(CacheTest, WriteTmpFile) {
  
}

// --------------------------------------------------------------------------
TEST_F(CacheTest, Resize) {

}

// --------------------------------------------------------------------------
TEST_F(CacheTest, ResizeTmpFile) {
  
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
