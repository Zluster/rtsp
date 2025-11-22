#include <gtest/gtest.h>
#include "base/logger.hpp"
#include "base/queue.hpp"
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

// 测试 Logger 管理器基本功能
TEST(LoggerTest, BasicFunctionality)
{
    // 重置日志系统
    base::LoggerManager::instance().reset();

    // 创建专用日志文件
    const std::string log_file = "test_basic.log";

    // 手动创建日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    // 测试日志输出
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_FATAL("Fatal message");

    // 验证日志级别设置
    base::LoggerManager::instance().set_log_level(base::LogLevel::WARN);
    LOG_DEBUG("This debug message should be filtered out");
    LOG_INFO("This info message should be filtered out");
    LOG_WARN("This warning should be logged");

    // 等待同步写入完成
    base::LoggerManager::instance().get_logger()->flush();

    // 验证文件内容
    std::ifstream file(log_file);
    ASSERT_TRUE(file.is_open());

    std::string content;
    std::string line;
    while (std::getline(file, line))
    {
        content += line + "\n";
    }
    file.close();

    // 检查日志级别过滤是否生效
    EXPECT_EQ(content.find("This debug message should be filtered out"), std::string::npos);
    EXPECT_EQ(content.find("This info message should be filtered out"), std::string::npos);
    EXPECT_NE(content.find("This warning should be logged"), std::string::npos);

    // 清理测试文件
    std::filesystem::remove(log_file);
    SUCCEED();
}

// 测试日志级别过滤
TEST(LoggerTest, LogLevelFiltering)
{
    // 重置日志系统
    base::LoggerManager::instance().reset();

    const std::string log_file = "test_level_filter.log";

    // 创建同步日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    // 设置日志级别为WARN，这样DEBUG和INFO会被过滤
    base::LoggerManager::instance().set_log_level(base::LogLevel::WARN);

    LOG_DEBUG("This debug should be filtered");
    LOG_INFO("This info should be filtered");
    LOG_WARN("This warning should appear");
    LOG_ERROR("This error should appear");

    // 等待同步写入完成
    base::LoggerManager::instance().get_logger()->flush();

    // 检查文件内容，确保只有WARN及以上级别的日志被写入
    std::ifstream file(log_file);
    EXPECT_TRUE(file.is_open());

    std::string content;
    std::string line;
    int line_count = 0;
    while (std::getline(file, line))
    {
        content += line + "\n";
        line_count++;
    }
    file.close();

    // 应该只有2行（WARN和ERROR）
    EXPECT_EQ(line_count, 2);
    EXPECT_NE(content.find("This warning should appear"), std::string::npos);
    EXPECT_NE(content.find("This error should appear"), std::string::npos);
    EXPECT_EQ(content.find("This debug should be filtered"), std::string::npos);
    EXPECT_EQ(content.find("This info should be filtered"), std::string::npos);

    // 清理测试文件
    std::filesystem::remove(log_file);
}

// 测试格式化字符串功能
TEST(LoggerTest, FormatString)
{
    // 重置日志系统
    base::LoggerManager::instance().reset();

    const std::string log_file = "test_format.log";

    // 创建同步日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    // 测试各种格式化参数
    LOG_INFO("Integer: %d", 42);
    LOG_INFO("Float: %.2f", 3.14159f);
    LOG_INFO("String: %s", "Hello World");
    LOG_INFO("Multiple: %d %s %.1f", 100, "test", 2.5);
    LOG_INFO("Multiple args: %d %s %.1f, %d, %s", 100, "test", 2.5, 11, "dsfsf");

    base::LoggerManager::instance().get_logger()->flush();

    // 检查文件内容
    std::ifstream file(log_file);
    EXPECT_TRUE(file.is_open());

    std::string content;
    std::string line;
    while (std::getline(file, line))
    {
        content += line + "\n";
    }
    file.close();

    EXPECT_NE(content.find("Integer: 42"), std::string::npos);
    EXPECT_NE(content.find("Float: 3.14"), std::string::npos);
    EXPECT_NE(content.find("String: Hello World"), std::string::npos);
    EXPECT_NE(content.find("Multiple: 100 test 2.5"), std::string::npos);
    EXPECT_NE(content.find("Multiple args: 100 test 2.5, 11, dsfsf"), std::string::npos);

    // 清理测试文件
    std::filesystem::remove(log_file);
}

// 测试多线程并发日志
TEST(LoggerTest, MultiThreadConcurrency)
{
    // 重置日志系统
    base::LoggerManager::instance().reset();

    const std::string log_file = "test_concurrent.log";

    // 创建异步日志器
    auto logger = std::make_unique<base::AsyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    std::vector<std::thread> threads;

    // 创建多个线程同时写日志
    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([t, log_file]()
                             {
            for (int i = 0; i < 100; ++i) {
                LOG_INFO("Thread %d, Message %d", t, i);
            } });
    }

    // 等待所有线程完成
    for (auto &t : threads)
    {
        t.join();
    }

    // 等待异步日志处理完成
    base::LoggerManager::instance().get_logger()->flush();

    // 检查文件行数（应该有400行）
    std::ifstream file(log_file);
    EXPECT_TRUE(file.is_open());

    int line_count = 0;
    std::string line;
    while (std::getline(file, line))
    {
        line_count++;
    }
    file.close();

    EXPECT_EQ(line_count, 400);

    // 清理测试文件
    std::filesystem::remove(log_file);
}

// 测试性能
TEST(LoggerTest, Performance)
{
    // 重置日志系统
    base::LoggerManager::instance().reset();

    const std::string log_file = "test_performance.log";

    // 创建异步日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    //  logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    // 记录开始时间
    auto start = std::chrono::high_resolution_clock::now();

    // 写入大量日志
    for (int i = 0; i < 1000000; ++i)
    {
        LOG_INFO("Performance test message %d", i);
    }

    // 记录结束时间
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Write time for 100,000 messages: " << duration << "ms" << std::endl;

    // 等待异步处理完成
    base::LoggerManager::instance().get_logger()->flush();

    // 检查性能（异步模式下10万条消息应该在合理时间内完成）
    EXPECT_LT(duration, 10000); // 10秒以内（更宽松）

    // 检查文件行数
    std::ifstream file(log_file);
    EXPECT_TRUE(file.is_open());

    int line_count = 0;
    std::string line;
    while (std::getline(file, line))
    {
        line_count++;
    }
    file.close();

    EXPECT_EQ(line_count, 1000000);

    // 清理测试文件
    //  std::filesystem::remove(log_file);
}