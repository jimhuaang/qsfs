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

#include <chrono>  // NOLINT
#include <future>  // NOLINT
#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "base/Logging.h"
#include "base/Utils.h"
#include "data/ResourceManager.h"

namespace QS {

namespace Data {

using std::future;
using std::packaged_task;
using std::unique_ptr;
using std::vector;
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

class ResourceManagerTest : public Test {
 protected:
  static void SetUpTestCase() { InitLog(); }
};

TEST_F(ResourceManagerTest, Default) {
  ResourceManager manager;
  EXPECT_FALSE(manager.ResourcesAvailable());
}

TEST_F(ResourceManagerTest, TestPutResource) {
  ResourceManager manager;
  manager.PutResource(Resource(new vector<char>(10)));
  EXPECT_TRUE(manager.ResourcesAvailable());

  for (auto &resource : manager.ShutdownAndWait(1)) {
    if (resource) {
      resource.reset();
    }
  }

  EXPECT_FALSE(manager.ResourcesAvailable());
}

TEST_F(ResourceManagerTest, TestAcquireReleaseResource) {
  ResourceManager manager;
  manager.PutResource(Resource(new vector<char>(10)));
  packaged_task<Resource()> task([&manager] { return manager.Acquire(); });
  future<Resource> f = task.get_future();
  task();
  auto status = f.wait_for(std::chrono::milliseconds(100));
  ASSERT_EQ(status, std::future_status::ready);
  EXPECT_FALSE(manager.ResourcesAvailable());

  auto resource = f.get();
  ASSERT_EQ(*(resource), vector<char>(10));

  manager.Release(std::move(resource));
  EXPECT_TRUE(manager.ResourcesAvailable());

  for (auto &resource : manager.ShutdownAndWait(1)) {
    if (resource) {
      resource.reset();
    }
  }
  EXPECT_FALSE(manager.ResourcesAvailable());
}

}  // namespace Data
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
