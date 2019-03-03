#include "fas/executorptr.h"
#include "fas/queueexecutor.h"
#include "fas/stream.h"
#include "fas/threadedexecutor.h"

#include "gtest/gtest.h"

TEST(Stream, StreamProducer)
{
  auto [stream, producer] = fas::makeStreamProducer<int>();
  ASSERT_TRUE(stream.isValid());
  ASSERT_FALSE(stream.isFinished());
  ASSERT_FALSE(stream.isReady());

  producer.send(1);
  ASSERT_FALSE(stream.isFinished());
  ASSERT_TRUE(stream.isReady());
  ASSERT_EQ(1, stream.get());

  producer.send(2);
  producer.reset();

  ASSERT_FALSE(stream.isFinished());
  ASSERT_TRUE(stream.isReady());
  ASSERT_EQ(2, stream.get());

  ASSERT_TRUE(stream.isFinished());
  ASSERT_TRUE(stream.isReady());
  ASSERT_EQ(std::nullopt, stream.get());
}

TEST(Stream, Continuation)
{
  auto executor = std::make_shared<fas::QueueExecutor>();
  fas::ExecutorPtr<fas::QueueExecutor> executorHandle(executor);

  auto [stream, producer] = fas::makeStreamProducer<int>();
  producer.send(10);
  producer.send(20);

  auto stream2 = std::move(stream)
                     .then(executorHandle, [](int i) { return std::make_pair(i, i * i); })
                     .then(executorHandle, [](const auto& v) { return v; })
                     .then(executorHandle, [](const auto& v) { return v; })
                     .then(executorHandle, [](const auto& v) { return v; });
  static_assert(std::is_same_v<decltype(stream2), fas::Stream<std::pair<int, int>>>, "Type is correctly deduced");

  ASSERT_FALSE(stream2.isReady());
  ASSERT_TRUE(stream2.isValid());
  ASSERT_EQ(1u, executor->jobCount());

  executor->run();
  ASSERT_TRUE(stream2.isReady());
  ASSERT_EQ(std::make_pair(10, 100), stream2.get());
  ASSERT_EQ(std::make_pair(20, 400), stream2.get());

  producer.send(30);
  producer.send(40);
  producer.send(50);
  producer.reset();

  // Even when the executor runs the callbacks in reverse order, the values should be received in the original order
  executor->runWrongOrder();
  ASSERT_TRUE(stream2.isReady());
  ASSERT_EQ(std::make_pair(30, 900), stream2.get());
  ASSERT_EQ(std::make_pair(40, 40 * 40), stream2.get());
  ASSERT_EQ(std::make_pair(50, 50 * 50), stream2.get());
  ASSERT_EQ(std::nullopt, stream2.get());
  ASSERT_EQ(std::nullopt, stream2.get());
}

// TODO: Cancellation

TEST(Stream, ThreadedExecutor)
{
  auto executor = std::make_shared<fas::ThreadedExecutor>();
  fas::ExecutorPtr<fas::ThreadedExecutor> executorHandle(executor);
  executor->start();

  auto [stream, producer] = fas::makeStreamProducer<int>();
  producer.send(10);
  producer.send(15);

  for (int i = 0; i < 100; ++i)
  {
    stream = std::move(stream).then(executorHandle, [](int i) { return i + 2; }).then(executorHandle, [](int i) {
      return i - 1;
    });
  }

  producer.reset();

  auto val = stream.get();
  ASSERT_TRUE(val.has_value());
  ASSERT_EQ(110, *val);
  ASSERT_FALSE(stream.isFinished());

  val = stream.get();
  ASSERT_TRUE(val.has_value());
  ASSERT_EQ(115, *val);

  val = stream.get();
  ASSERT_TRUE(stream.isFinished());
  ASSERT_FALSE(val.has_value());
}
