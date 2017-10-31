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
#include <functional>
#include <future>  // NOLINT
#include <memory>

#include "gtest/gtest.h"

#include "base/TaskHandle.h"
#include "base/ThreadPool.h"

namespace QS {

namespace Threading {

// In order to test ThreadPool private members, need to define
// test fixture and tests in the same namespace with ThreadPool,
// so they can be friends of class ThreadPool.

using std::function;
using std::future;
using std::make_shared;
using std::packaged_task;
using std::shared_ptr;
using ::testing::Test;

// Return n!. For negative, n! is defined to be 1;
int Factorial(int n) {
  int result = 1;
  for (int i = 1; i <= n; ++i) {
    result *= i;
  }
  return result;
}

int Add(int a, int b) { return a + b; }

static const int poolSize_ = 4;

class ThreadPoolTest : public Test {
 public:
  future<int> FactorialCallable(int n) {
    auto pTask =
        make_shared<packaged_task<int()>>([n] { return Factorial(n); });
    m_pThreadPool->SubmitToThread(Task([pTask]() { (*pTask)(); }));
    return pTask->get_future();
  }

  future<int> FactorialCallablePrioritized(int n) {
    auto pTask =
        make_shared<packaged_task<int()>>([n] { return Factorial(n); });
    m_pThreadPool->SubmitToThread(Task([pTask]() { (*pTask)(); }), true);
    return pTask->get_future();
  }

 protected:
  void SetUp() override { m_pThreadPool = new ThreadPool(poolSize_); }

  void TearDown() override { delete m_pThreadPool; }

 protected:
  ThreadPool *m_pThreadPool;
};

TEST_F(ThreadPoolTest, TestSubmit) {
  int num = 5;
  auto f = FactorialCallable(num);
  auto fStatus = f.wait_for(std::chrono::milliseconds(100));
  ASSERT_EQ(fStatus, std::future_status::ready);
  EXPECT_EQ(f.get(), 120);

  auto f1 = FactorialCallablePrioritized(num);
  auto fStatus1 = f1.wait_for(std::chrono::milliseconds(100));
  ASSERT_EQ(fStatus1, std::future_status::ready);
  EXPECT_EQ(f1.get(), 120);
}

TEST_F(ThreadPoolTest, TestSubmitAsync) {
  auto callback = [](int resultOfFactorial, int num) {
    EXPECT_EQ(num, 5);
    EXPECT_EQ(resultOfFactorial, 120);
  };
  m_pThreadPool->SubmitAsync(callback, Factorial, 5);

  auto callback1 = [](int resultOfAdd, int a, int b) {
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 11);
    EXPECT_EQ(resultOfAdd, 12);
  };
  m_pThreadPool->SubmitAsyncPrioritized(callback1, Add, 1, 11);
}

struct AsyncContext {};

TEST_F(ThreadPoolTest, TestSubmitAsyncWithContext) {
  AsyncContext asyncContext;
  auto callback = [](AsyncContext context, int resultOfFactorial, int num) {
    EXPECT_EQ(num, 5);
    EXPECT_EQ(resultOfFactorial, 120);
  };
  m_pThreadPool->SubmitAsyncWithContext(callback, asyncContext, Factorial, 5);

  auto callback1 = [](AsyncContext context, int resultOfAdd, int a, int b) {
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 11);
    EXPECT_EQ(resultOfAdd, 12);
  };
  m_pThreadPool->SubmitAsyncWithContextPrioritized(callback1, asyncContext, Add,
                                                   1, 11);
}

}  // namespace Threading
}  // namespace QS

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int code = RUN_ALL_TESTS();
  return code;
}
