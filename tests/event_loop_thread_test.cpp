#include <gtest/gtest.h>
#include "net/EventLoopThread.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace net;

// 测试 EventLoopThread 启动后是否正确创建 EventLoop
TEST(EventLoopThreadTest, StartLoop)
{
    EventLoopThread thread;
    EventLoop *loop = thread.startLoop(); // 启动线程并获取 loop
    EXPECT_NE(loop, nullptr);

    // 使用 isInLoopThread 方法代替直接访问 threadId_
    bool isInLoopThread = false;
    loop->runInLoop([&]()
                    { isInLoopThread = loop->isInLoopThread(); });

    // 给一些时间让任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(isInLoopThread); // loop 在其自己的线程中运行

    thread.stopLoop(); // 停止线程
}

// 测试线程初始化回调
TEST(EventLoopThreadTest, InitCallback)
{
    std::atomic<bool> callbackCalled(false);
    std::thread::id callbackThreadId;
    std::thread::id loopThreadId;

    EventLoopThread thread([&](EventLoop *loop)
                           {
                               callbackCalled = true;
                               callbackThreadId = std::this_thread::get_id();
                               // 我们无法直接访问 loop->threadId_，但可以通过 isInLoopThread 验证
                               EXPECT_TRUE(loop->isInLoopThread()); },
                           "TestThread");

    EventLoop *loop = thread.startLoop();
    (void)loop;
    // 等待回调执行完成
    while (!callbackCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    thread.stopLoop();
    EXPECT_TRUE(callbackCalled);
}

// 测试线程安全退出
TEST(EventLoopThreadTest, StopLoop)
{
    EventLoopThread thread;
    EventLoop *loop = thread.startLoop();
    std::atomic<bool> loopRunning(true);

    loop->runInLoop([&]()
                    { loopRunning = true; });

    // 等待前面的任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    thread.stopLoop(); // 停止线程

    // 验证 loop 已退出
    loop->runInLoop([&]() { // 此时 loop 已退出，任务不会执行
        loopRunning = false;
    });

    // 给一些时间让测试完成
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(loopRunning);
}