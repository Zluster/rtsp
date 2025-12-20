#include <gtest/gtest.h>
#include "net/EventLoop.hpp"
#include "net/Channel.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace net;

// 测试 EventLoop 启动和退出
TEST(EventLoopTest, StartAndQuit)
{
    EventLoop loop;
    std::thread t([&loop]()
                  {
                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                      loop.quit(); // 子线程中退出事件循环
                  });
    loop.loop(); // 启动事件循环
    t.join();
    EXPECT_FALSE(loop.isInLoopThread()); // 验证当前线程不是事件循环线程
}

// 测试 runInLoop 和 queueInLoop 跨线程任务调度
TEST(EventLoopTest, RunInLoop)
{
    EventLoop loop;
    std::atomic<bool> called(false);
    std::thread t([&loop, &called]()
                  { loop.runInLoop([&called]()
                                   { called = true; }); });
    t.join();
    loop.loop(); // 执行任务后退出
    EXPECT_TRUE(called);
}

// 测试定时器功能（runAfter）
TEST(EventLoopTest, Timer)
{
    EventLoop loop;
    std::atomic<int> count(0);
    loop.runAfter(0.1, [&count, &loop]() { // 100ms 后执行
        count++;
        loop.quit();
    });
    loop.loop();
    EXPECT_EQ(count, 1);
}

// 测试 Channel 注册和事件触发
TEST(EventLoopTest, Channel)
{
    EventLoop loop;
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    Channel channel(&loop, pipefd[0]);
    std::atomic<bool> readTriggered(false);

    channel.setReadCallback([&](Timestamp)
                            {
        readTriggered = true;
        loop.quit(); });
    channel.enableReading(); // 注册读事件

    // 向管道写数据，触发读事件
    std::thread t([&]()
                  {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        [[maybe_unused]] ssize_t bytes_written = write(pipefd[1], "test", 4);
        assert(bytes_written == 4); });
    loop.loop();
    t.join();
    close(pipefd[0]);
    close(pipefd[1]);
    EXPECT_TRUE(readTriggered);
}