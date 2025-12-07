#include <gtest/gtest.h>
#include "base/Timer.hpp"
#include <thread>
#include <chrono>

// 测试 Timer 基本功能
TEST(TimerTest, BasicFunctionality)
{
    bool callback_called = false;

    // 创建一个 Timer
    base::Timer timer([&callback_called]()
                      { callback_called = true; }, base::Timestamp::now(), 0.1); // 10 毫秒到期

    // 运行 timer
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 等待足够的时间来触发
    timer.run();                                                 // 手动调用 run() 以模拟 Timer 的到期

    EXPECT_TRUE(callback_called); // 检查回调函数是否被调用
}

// 测试定时器的 restart 方法
TEST(TimerTest, RestartFunctionality)
{
    int call_count = 0;

    auto callback = [&call_count]()
    {
        ++call_count;
    };
    base::Timer timer(callback, base::Timestamp::now(), 0.1); // 10毫秒到期

    // 第一次运行
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    timer.run();
    EXPECT_EQ(call_count, 1); // 检查第一次调用

    // 重启定时器
    timer.restart(base::Timestamp::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    timer.run();

    EXPECT_EQ(call_count, 2); // 检查第二次调用
}

// 测试 TimerQueue 添加和处理定时器
TEST(TimerQueueTest, AddAndHandleTimer)
{
    base::TimerQueue timerQueue;
    int callback_count = 0;

    // 添加一个一次性定时器
    timerQueue.addTimer([&callback_count]()
                        { callback_count++; }, base::Timestamp::now() + 0.1, 0); // 10 毫秒后到期

    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 等待足够的时间
    timerQueue.handleExpiredTimers();                            // 处理过期的定时器

    EXPECT_EQ(callback_count, 1); // 验证回调是否被调用
}

// 测试 TimerQueue 取消定时器
TEST(TimerQueueTest, CancelTimer)
{
    base::TimerQueue timerQueue;
    int callback_count = 0;

    // 添加一个定时器
    base::TimerId timerId = timerQueue.addTimer([&callback_count]()
                                                { callback_count++; }, base::Timestamp::now() + 0.1, 0);

    // 取消定时器
    timerQueue.cancel(timerId);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    timerQueue.handleExpiredTimers(); // 尝试处理过期的定时器

    EXPECT_EQ(callback_count, 0); // 验证回调没有被调用
}

// 测试带有重复的定时器
TEST(TimerTest, RepeatingTimer)
{
    int call_count = 0;

    auto callback = [&call_count]()
    {
        ++call_count;
    };

    // 创建一个每 10 毫秒重复的定时器
    base::Timer timer(callback, base::Timestamp::now() + 0.1, 0.1);

    // 运行定时器
    for (int i = 0; i < 3; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        timer.run();
    }

    EXPECT_GE(call_count, 2); // 检查至少调用了两次，可能是因为定时器请求重复调用
}
