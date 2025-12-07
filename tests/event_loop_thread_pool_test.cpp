#include <gtest/gtest.h>
#include "net/EventLoopThreadPool.hpp"
#include "net/EventLoop.hpp"
#include <unordered_set>
#include <thread>
#include <chrono>

using namespace net;

// 测试线程池初始化（0线程和多线程）
TEST(EventLoopThreadPoolTest, Init)
{
    EventLoop baseLoop;
    // 测试 0 线程（仅使用 baseLoop）
    EventLoopThreadPool pool0(&baseLoop, "Pool0");
    pool0.setThreadNum(0);
    pool0.start();
    EXPECT_EQ(pool0.getNextLoop(), &baseLoop);

    // 测试 3 线程
    EventLoopThreadPool pool3(&baseLoop, "Pool3");
    pool3.setThreadNum(3);
    pool3.start();
    std::unordered_set<EventLoop *> loops;
    for (int i = 0; i < 10; ++i)
    {
        loops.insert(pool3.getNextLoop());
    }
    EXPECT_EQ(loops.size(), 3); // 轮询分配，共3个loop
    pool3.getAllLoops();        // 验证所有loop可获取
}

// 测试 getNextLoop 轮询分配
TEST(EventLoopThreadPoolTest, GetNextLoop)
{
    EventLoop baseLoop;
    EventLoopThreadPool pool(&baseLoop, "PollNext");
    pool.setThreadNum(2);
    pool.start();

    EventLoop *loop1 = pool.getNextLoop();
    EventLoop *loop2 = pool.getNextLoop();
    EventLoop *loop3 = pool.getNextLoop(); // 轮询到第一个
    EXPECT_NE(loop1, loop2);
    EXPECT_EQ(loop1, loop3);
}

// 测试 getLoopForHash 哈希分配
TEST(EventLoopThreadPoolTest, GetLoopForHash)
{
    EventLoop baseLoop;
    EventLoopThreadPool pool(&baseLoop, "PollHash");
    pool.setThreadNum(2);
    pool.start();

    // 相同哈希值应分配到同一个loop
    size_t hash1 = 100;
    size_t hash2 = 101;
    EXPECT_EQ(pool.getLoopForHash(hash1), pool.getLoopForHash(hash1));
    EXPECT_NE(pool.getLoopForHash(hash1), pool.getLoopForHash(hash2));
}

// 测试线程池初始化回调
TEST(EventLoopThreadPoolTest, ThreadInitCallback)
{
    EventLoop baseLoop;
    std::atomic<int> callbackCount(0);
    EventLoopThreadPool pool(&baseLoop, "PoolCallback");
    pool.setThreadNum(2);
    pool.start([&](EventLoop *)
               { callbackCount++; });
    // 等待回调执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(callbackCount, 2); // 2个线程，回调执行2次
}