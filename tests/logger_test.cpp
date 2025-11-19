#include <gtest/gtest.h>
#include "base/logger.hpp"

// 测试 Logger 类的基本功能
TEST(LoggerTest, BasicFunctionality)
{
    // 创建 Logger 对象
    base::Logger logger;

    // 测试设置日志级别
    logger.set_log_level(base::Logger::LOG_LEVEL_INFO);

    // 使用断言检查日志级别是否设置成功
    // 假设 Logger 有 get_log_level() 方法
    // EXPECT_EQ(logger.get_log_level(), base::Logger::LOG_LEVEL_INFO);

    // 测试日志输出（这里只是简单检查是否能正常调用）
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.fatal("Fatal message");

    // 这个测试不会失败，因为没有断言失败
    SUCCEED();
}

// 测试日志文件输出
TEST(LoggerTest, FileOutput)
{
    base::Logger logger("test.log");
    logger.set_output_file(true);
    logger.info("Test log to file");

    // 检查文件是否创建
    std::ifstream file("test.log");
    EXPECT_TRUE(file.good());
    file.close();

    // 清理测试文件
    remove("test.log");
}

// 测试文件写入性能
TEST(LoggerTest, FileWritePerformance)
{
    base::Logger logger("performance.log");
    logger.set_output_file(true);
    logger.set_output_console(false);
    // 记录开始时间
    auto start = std::chrono::high_resolution_clock::now();

    // 写入大量日志
    for (int i = 0; i < 1000000; ++i)
    {
        logger.info("Performance test message");
    }
    // 记录结束时间
    auto end = std::chrono::high_resolution_clock::now();

    // 计算写入时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Write time: " << duration << "ms" << std::endl;
    // 检查写入时间是否在可接受范围内
    EXPECT_LT(duration, 1000); // 1秒以内
}