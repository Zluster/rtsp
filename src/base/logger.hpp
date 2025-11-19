#pragma once

#include <mutex>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>
namespace base
{

    class Logger
    {
    public:
        enum LogLevel
        {
            LOG_LEVEL_DEBUG,
            LOG_LEVEL_INFO,
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
            LOG_LEVEL_FATAL,
            LOG_LEVEL_COUNT
        };

        Logger();
        explicit Logger(const std::string &filename);
        ~Logger();

        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;
        Logger(Logger &&) = delete;
        Logger &operator=(Logger &&) = delete;

        void set_log_file(const std::string &filename);
        void set_log_level(LogLevel level);
        void set_output_console(bool enable);
        void set_output_file(bool enable);

        void set_file_roll_policy(size_t max_size, int backup_count);

        void log(const std::string &message, LogLevel level);

        template <typename... Args>
        void debug(Args &&...args)
        {
            log(format_message(std::forward<Args>(args)...), LOG_LEVEL_DEBUG);
        }
        template <typename... Args>
        void info(Args &&...args)
        {
            log(format_message(std::forward<Args>(args)...), LOG_LEVEL_INFO);
        }

        template <typename... Args>
        void warning(Args &&...args)
        {
            log(format_message(std::forward<Args>(args)...), LOG_LEVEL_WARN);
        }

        template <typename... Args>
        void error(Args &&...args)
        {
            log(format_message(std::forward<Args>(args)...), LOG_LEVEL_ERROR);
        }

        template <typename... Args>
        void fatal(Args &&...args)
        {
            log(format_message(std::forward<Args>(args)...), LOG_LEVEL_FATAL);
        }

    private:
        template <typename T>
        void format_message_helper(std::ostringstream &oss, T &&args)
        {
            oss << std::forward<T>(args);
        }

        template <typename T, typename... Args>
        void format_message_helper(std::ostringstream &oss, T &&arg, Args &&...args)
        {
            oss << std::forward<T>(arg) << " ";
            format_message_helper(oss, std::forward<Args>(args)...);
        }

        template <typename... Args>
        std::string format_message(Args &&...args)
        {
            std::ostringstream oss;
            format_message_helper(oss, std::forward<Args>(args)...);
            return oss.str();
        }
        void check_and_roll_file();

    private:
        LogLevel log_level_;            // 当前日志级别
        std::string log_file_path_;     // 日志文件路径
        std::ofstream log_file_stream_; // 日志文件流
        bool output_to_console_;        // 是否输出到控制台
        bool output_to_file_;           // 是否输出到文件
        size_t file_max_size_;          // 单个日志文件最大大小
        int file_backup_count_;         // 日志文件备份数量
        std::mutex log_mutex_;          // 日志互斥锁，保证线程安全

        static const char *LOG_LEVEL_STRINGS[];
    };
} // namespace base