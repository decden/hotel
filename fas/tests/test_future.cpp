#include "fas/executorptr.h"
#include "fas/future.h"
#include "fas/queueexecutor.h"
#include "fas/threadedexecutor.h"
#include "fas/fas.h"
#include "fas/systemexecutor.h"

#include "gtest/gtest.h"

TEST(Future, Promise)
{
  auto [future, promise] = fas::makePromise<int>();
  ASSERT_FALSE(future.isReady());
  ASSERT_TRUE(future.isValid());

  promise.resolve(10);
  ASSERT_TRUE(future.isReady());
  ASSERT_EQ(10, future.get());
}

TEST(Future, ImplicitCancellation)
{
  auto executor = std::make_shared<fas::QueueExecutor>();
  fas::ExecutorPtr<fas::QueueExecutor> executorHandle(executor);

  bool executed1 = false;
  bool executed2 = false;

  auto [future, promise] = fas::makePromise<int>();
  future = std::move(future).then(executorHandle, [&](int i) { executed1 = true; return i; });
  future = std::move(future).then(executorHandle, [&](int i) { executed2 = true; return i; });
  promise.resolve(10);

  future.reset();
  executor->run();
  ASSERT_FALSE(executed1);
  ASSERT_FALSE(executed2);
}

TEST(Future, PromiseContinuation)
{
  auto executor = std::make_shared<fas::QueueExecutor>();
  fas::ExecutorPtr<fas::QueueExecutor> executorHandle(executor);

  auto [future, promise] = fas::makePromise<int>();
  promise.resolve(10);

  auto future2 = std::move(future).then(executorHandle, [](int i) { return std::make_pair(i, i * i); });
  static_assert(std::is_same_v<decltype(future2), fas::Future<std::pair<int, int>>>, "Type is correctly deduced");

  ASSERT_FALSE(future2.isReady());
  ASSERT_TRUE(future2.isValid());
  ASSERT_EQ(1u, executor->jobCount());

  executor->run();
  ASSERT_TRUE(future2.isReady());
  ASSERT_EQ(std::make_pair(10, 100), future2.get());
}  

TEST(Future, ThreadedExecutor)
{
  auto executor = std::make_shared<fas::ThreadedExecutor>();
  fas::ExecutorPtr<fas::ThreadedExecutor> executorHandle(executor);
  executor->start();

  auto [future, promise] = fas::makePromise<int>();
  promise.resolve(10);

  for (int i = 0; i < 100; ++i)
    future = std::move(future)
      .then(executorHandle, [](int i) { return i + 2; })
      .then(executorHandle, [](int i) { return i - 1; });

  ASSERT_EQ(110, future.get());
}

TEST(Future, SystemExecutor)
{
  fas::initialize();
  auto executor = fas::SystemExecutor{};

  auto [future, promise] = fas::makePromise<int>();
  promise.resolve(10);

  for (int i = 0; i < 100; ++i)
    future = std::move(future)
      .then(executor, [](int i) { return i + 2; })
      .then(executor, [](int i) { return i - 1; });

  ASSERT_EQ(110, future.get());
  fas::shutdown();
}

TEST(Future, MultipleExecutors)
{
  auto executor1 = std::make_shared<fas::QueueExecutor>();
  fas::ExecutorPtr<fas::QueueExecutor> executorHandle1(executor1);
  auto executor2 = std::make_shared<fas::QueueExecutor>();
  fas::ExecutorPtr<fas::QueueExecutor> executorHandle2(executor2);

  auto [future, promise] = fas::makePromise<int>();
  auto future2 = std::move(future).then(executorHandle1, [](int i) { return i * 2; });
  auto future3 = std::move(future2).then(executorHandle2, [](int i) { return i * 3; });
  auto future4 = std::move(future3).then(executorHandle1, [](int i) { return i * 4; });

  promise.resolve(10);

  ASSERT_EQ(1u, executor1->jobCount());
  ASSERT_EQ(0u, executor2->jobCount());

  executor1->run();
  ASSERT_EQ(0u, executor1->jobCount());
  ASSERT_EQ(1u, executor2->jobCount());

  executor2->run();
  ASSERT_EQ(1u, executor1->jobCount());
  ASSERT_EQ(0u, executor2->jobCount());

  executor1->run();
  ASSERT_EQ(0u, executor1->jobCount());
  ASSERT_EQ(0u, executor2->jobCount());

  ASSERT_TRUE(future4.isReady());
  ASSERT_EQ(10 * 2 * 3 * 4, future4.get());
}
