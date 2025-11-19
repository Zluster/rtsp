#include "logger.hpp"

namespace base
{
    const char *Logger::LOG_LEVEL_STRINGS[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

    Logger::Logger() : log_level_(LOG_LEVEL_INFO),
                       output_to_console_(true),
                       output_to_file_(false),
                       file_max_size_(1 * 1024 * 1024), // 默认单个文件最大10MB
                       file_backup_count_(5)            // 默认最多备份5个文件
    {
        log_file_path_ = "./app.log";
    }

    Logger::Logger(const std::string &file_path)
        : Logger()
    {
        log_file_path_ = file_path;
        output_to_file_ = true;
    }

    Logger::~Logger()
    {
        if (log_file_stream_.is_open())
        {
            log_file_stream_.close();
        }
    }

    // 设置日志级别
    void Logger::set_log_level(LogLevel level)
    {
        if (level >= 0 && level < LOG_LEVEL_COUNT)
        {
            log_level_ = level;
        }
    }

    // 设置日志文件路径
    void Logger::set_log_file(const std::string &file_path)
    {
        std::lock_guard<std::mutex> lock(log_mutex_);

        log_file_path_ = file_path;
        output_to_file_ = true;

        // 如果文件已打开，先关闭
        if (log_file_stream_.is_open())
        {
            log_file_stream_.close();
        }
    }

    // 设置是否输出到控制台
    void Logger::set_output_console(bool enable)
    {
        output_to_console_ = enable;
    }

    // 设置是否输出到文件
    void Logger::set_output_file(bool enable)
    {
        output_to_file_ = enable;
    }

    // 设置日志文件滚动策略
    void Logger::set_file_roll_policy(size_t max_size, int backup_count)
    {
        if (max_size > 0)
        {
            file_max_size_ = max_size;
        }
        if (backup_count > 0)
        {
            file_backup_count_ = backup_count;
        }
    }
    void Logger::log(const std::string &message, LogLevel level)
    {
        // 如果日志级别低于当前设置的级别，则忽略
        if (level < log_level_)
        {
            return;
        }

        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_tm = *std::localtime(&now_time_t);

        // 获取毫秒级时间
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // 构建日志消息
        std::ostringstream oss;

        // 格式化时间：YYYY-MM-DD HH:MM:SS.sss
        oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << now_ms.count()
            << " [" << LOG_LEVEL_STRINGS[level] << "] "
            << message << std::endl;

        std::string log_msg = oss.str();

        // 线程安全地输出日志
        std::lock_guard<std::mutex> lock(log_mutex_);

        // 输出到控制台
        if (output_to_console_)
        {
            // 根据日志级别设置控制台颜色
            switch (level)
            {
            case LOG_LEVEL_DEBUG:
                std::cout << "\033[37m"; // 白色
                break;
            case LOG_LEVEL_INFO:
                std::cout << "\033[32m"; // 绿色
                break;
            case LOG_LEVEL_WARN:
                std::cout << "\033[33m"; // 黄色
                break;
            case LOG_LEVEL_ERROR:
                std::cout << "\033[31m"; // 红色
                break;
            case LOG_LEVEL_FATAL:
                std::cout << "\033[41;37m"; // 红底白字
                break;
            default:
                break;
            }

            std::cout << log_msg;

            // 恢复默认颜色
            std::cout << "\033[0m";
        }

        // 输出到文件
        if (output_to_file_ && !log_file_path_.empty())
        {
            // 检查是否需要滚动日志文件
            check_and_roll_file();

            // 如果文件未打开，则尝试打开
            if (!log_file_stream_.is_open())
            {
                log_file_stream_.open(log_file_path_, std::ios::out | std::ios::app);
            }

            // 写入日志
            if (log_file_stream_.is_open())
            {
                log_file_stream_ << log_msg;
                log_file_stream_.flush(); // 立即写入磁盘
            }
        }
    }

    // 检查并滚动日志文件
    void Logger::check_and_roll_file()
    {
        if (!log_file_stream_.is_open())
        {
            return;
        }

        // 获取当前文件大小
        log_file_stream_.seekp(0, std::ios::end);
        size_t current_size = log_file_stream_.tellp();

        // 如果文件大小超过最大值，则滚动
        if (current_size >= file_max_size_)
        {
            log_file_stream_.close();

            // 备份现有日志文件
            for (int i = file_backup_count_ - 1; i > 0; --i)
            {
                std::string src = log_file_path_ + "." + std::to_string(i);
                std::string dst = log_file_path_ + "." + std::to_string(i + 1);
                std::rename(src.c_str(), dst.c_str());
            }

            // 重命名当前日志文件为 .1
            if (file_backup_count_ > 0)
            {
                std::string backup_path = log_file_path_ + ".1";
                std::rename(log_file_path_.c_str(), backup_path.c_str());
            }
        }
    }

}