#pragma once

#include <mutex>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <functional>
#include <string>
#include <utility>
#include <memory>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdarg>
#include "Queue.hpp"
namespace base
{
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        COUNT
    };

    enum class LogMode
    {
        SYNC,
        ASYNC
    };

    class LogSink
    {
    public:
        virtual ~LogSink() = default;
        virtual void log(const std::string &msg, LogLevel level) = 0;
        virtual void flush() = 0;
    };

    class ConsoleSink : public LogSink
    {
    public:
        ConsoleSink() = default;
        void log(const std::string &msg, LogLevel level) override;
        void flush() override;
    };

    class FileSink : public LogSink
    {
    public:
        FileSink(const std::string &filename, size_t max_size = 10 * 1024 * 1024, int max_files = 5);
        ~FileSink() override;
        void log(const std::string &msg, LogLevel level) override;
        void flush() override;

    private:
        void open_file();
        void rotate_file();

    private:
        std::string filename_;
        size_t max_size_;
        int max_files_;
        int file_handle_;
        std::atomic<size_t> current_size_;
        mutable std::mutex file_mutex_;
    };

    class LogMessage
    {
    public:
        LogMessage() = default;
        LogMessage(const std::string &msg, LogLevel level);
        LogMessage(const LogMessage &other) = delete;
        LogMessage(LogMessage &&other) noexcept;
        LogMessage &operator=(LogMessage &&other) noexcept;

        const std::string &get_raw_message() const;
        LogLevel get_level() const;
        auto get_timestamp() const;
        const std::string &get_formatted_message() const;

    private:
        std::string raw_message_;
        LogLevel level_;
        std::chrono::system_clock::time_point timestamp_;
        std::string formatted_message_;

        // 时间缓存相关
        static thread_local std::tm cached_tm_;
        static thread_local std::time_t last_time_t_;
        static std::mutex time_format_mutex_;
        static const char *LOG_LEVEL_STRINGS[];
    };

    class Logger
    {
    public:
        explicit Logger(LogMode mode = LogMode::SYNC);
        virtual ~Logger() = default;

        virtual void log(LogLevel level, const std::string &msg) = 0;
        virtual void flush() = 0;
        virtual void add_sink(std::shared_ptr<LogSink> sink) = 0;
        virtual void set_log_level(LogLevel level) = 0;
    };

    class SyncLogger : public Logger
    {
    public:
        explicit SyncLogger();
        void log(LogLevel level, const std::string &msg) override;
        void flush() override;
        void add_sink(std::shared_ptr<LogSink> sink) override;
        void set_log_level(LogLevel level) override;

    private:
        std::vector<std::shared_ptr<LogSink>> sinks_;
        LogLevel log_level_;
    };

    class AsyncLogger : public Logger
    {
    private:
        using LogQueue = LockFreeQueue<LogMessage>;

    public:
        explicit AsyncLogger();
        ~AsyncLogger() override;
        void log(LogLevel level, const std::string &msg) override;
        void flush() override;
        void add_sink(std::shared_ptr<LogSink> sink) override;
        void set_log_level(LogLevel level) override;

    private:
        void worker_thread();

    private:
        LogQueue log_queue_;
        std::vector<std::shared_ptr<LogSink>> sinks_;
        std::mutex sinks_mutex_;
        LogLevel log_level_;
        std::atomic<bool> stop_flag_;
        std::thread worker_thread_;
    };

    class LoggerManager
    {
    public:
        static LoggerManager &instance();

        void set_logger(std::unique_ptr<Logger> logger);
        Logger *get_logger();

        void set_log_level(LogLevel level);
        void add_sink(std::shared_ptr<LogSink> sink);
        void reset();

    private:
        LoggerManager() = default;
        ~LoggerManager() = default;
        LoggerManager(const LoggerManager &) = delete;
        LoggerManager &operator=(const LoggerManager &) = delete;

    private:
        std::unique_ptr<Logger> logger_;
        std::mutex mutex_;
    };

    inline std::string safe_printf_format(const char *fmt, ...)
    {
        va_list args;
        va_list args_copy;
        va_start(args, fmt);
        va_copy(args_copy, args);

        // 获取所需缓冲区大小
        int size = vsnprintf(nullptr, 0, fmt, args) + 1;
        va_end(args);

        if (size <= 1)
            return std::string(fmt);

        // 分配缓冲区并格式化
        std::unique_ptr<char[]> buffer(new char[size]);
        int written = vsnprintf(buffer.get(), size, fmt, args_copy);
        va_end(args_copy);

        if (written < 0)
            return std::string(fmt);

        return std::string(buffer.get(), written);
    }
} // namespace base

#define LOG_DEBUG(fmt, ...) base::LoggerManager::instance().get_logger()->log(base::LogLevel::DEBUG, base::safe_printf_format(fmt, ##__VA_ARGS__))
#define LOG_INFO(fmt, ...) base::LoggerManager::instance().get_logger()->log(base::LogLevel::INFO, base::safe_printf_format(fmt, ##__VA_ARGS__))
#define LOG_WARN(fmt, ...) base::LoggerManager::instance().get_logger()->log(base::LogLevel::WARN, base::safe_printf_format(fmt, ##__VA_ARGS__))
#define LOG_ERROR(fmt, ...) base::LoggerManager::instance().get_logger()->log(base::LogLevel::ERROR, base::safe_printf_format(fmt, ##__VA_ARGS__))
#define LOG_FATAL(fmt, ...) base::LoggerManager::instance().get_logger()->log(base::LogLevel::FATAL, base::safe_printf_format(fmt, ##__VA_ARGS__))
