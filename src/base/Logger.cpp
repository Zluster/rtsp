#include "Logger.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>
#include <cstdarg>

namespace base
{
    // 静态成员初始化
    thread_local std::tm LogMessage::cached_tm_ = {};
    thread_local std::time_t LogMessage::last_time_t_ = 0;
    std::mutex LogMessage::time_format_mutex_;
    const char *LogMessage::LOG_LEVEL_STRINGS[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    // ConsoleSink 实现
    void ConsoleSink::log(const std::string &msg, LogLevel level)
    {
        static std::mutex console_mutex;

        std::lock_guard<std::mutex> lock(console_mutex);
        switch (level)
        {
        case LogLevel::DEBUG:
            std::cout << "\033[37m" << msg << "\033[0m";
            break;
        case LogLevel::INFO:
            std::cout << "\033[32m" << msg << "\033[0m";
            break;
        case LogLevel::WARN:
            std::cout << "\033[33m" << msg << "\033[0m";
            break;
        case LogLevel::ERROR:
            std::cout << "\033[31m" << msg << "\033[0m";
            break;
        case LogLevel::FATAL:
            std::cout << "\033[41;37m" << msg << "\033[0m";
            break;
        default:
            std::cout << msg;
            break;
        }
        std::cout.flush();
    }

    void ConsoleSink::flush()
    {
        std::cout.flush();
    }

    // FileSink 实现
    FileSink::FileSink(const std::string &filename, size_t max_size, int max_files)
        : filename_(filename), max_size_(max_size), max_files_(max_files), file_handle_(-1), current_size_(0)
    {
        open_file();
    }

    FileSink::~FileSink()
    {
        if (file_handle_ != -1)
        {
            ::close(file_handle_);
        }
    }

    void FileSink::log(const std::string &msg, LogLevel level)
    {
        (void)level;
        std::lock_guard<std::mutex> lock(file_mutex_);

        if (file_handle_ == -1)
            return;

        // 检查文件大小并滚动
        if (current_size_ >= max_size_ && max_size_ > 0)
        {
            rotate_file();
        }

        // 直接写入文件描述符，避免流操作的开销
        size_t written = 0;
        const char *data = msg.c_str();
        size_t len = msg.length();

        while (written < len)
        {
            ssize_t result = ::write(file_handle_, data + written, len - written);
            if (result < 0)
                break;
            written += result;
        }

        current_size_ += len;
    }

    void FileSink::flush()
    {
        if (file_handle_ != -1)
        {
            ::fsync(file_handle_);
        }
    }

    void FileSink::open_file()
    {
        file_handle_ = ::open(filename_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

        if (file_handle_ != -1)
        {
            struct stat st;
            if (::fstat(file_handle_, &st) == 0)
            {
                current_size_ = st.st_size;
            }
            else
            {
                current_size_ = 0;
            }
        }
    }

    void FileSink::rotate_file()
    {
        if (file_handle_ != -1)
        {
            ::close(file_handle_);
        }

        // 滚动备份文件
        for (int i = max_files_ - 1; i > 0; --i)
        {
            std::string src = filename_ + "." + std::to_string(i);
            std::string dst = filename_ + "." + std::to_string(i + 1);
            if (std::filesystem::exists(src))
            {
                std::filesystem::rename(src, dst);
            }
        }

        // 重命名当前文件
        if (max_files_ > 0)
        {
            std::string backup_path = filename_ + ".1";
            std::filesystem::rename(filename_, backup_path);
        }

        // 重新打开文件
        open_file();
    }

    // LogMessage 实现
    LogMessage::LogMessage(const std::string &msg, LogLevel level)
        : level_(level), timestamp_(std::chrono::system_clock::now())
    {
        raw_message_ = msg;

        // 预格式化消息以避免在异步线程中重复格式化
        auto now_time_t = std::chrono::system_clock::to_time_t(timestamp_);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timestamp_.time_since_epoch()) %
                      1000;

        if (now_time_t != last_time_t_)
        {
            std::lock_guard<std::mutex> lock(time_format_mutex_);
            cached_tm_ = *std::localtime(&now_time_t);
            last_time_t_ = now_time_t;
        }

        std::ostringstream oss;
        oss << std::put_time(&cached_tm_, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << now_ms.count()
            << " [" << LOG_LEVEL_STRINGS[static_cast<int>(level_)] << "] "
            << raw_message_ << "\n";

        formatted_message_ = oss.str();
    }

    LogMessage::LogMessage(LogMessage &&other) noexcept
        : raw_message_(std::move(other.raw_message_)),
          level_(other.level_),
          timestamp_(other.timestamp_),
          formatted_message_(std::move(other.formatted_message_)) {}

    LogMessage &LogMessage::operator=(LogMessage &&other) noexcept
    {
        if (this != &other)
        {
            raw_message_ = std::move(other.raw_message_);
            level_ = other.level_;
            timestamp_ = other.timestamp_;
            formatted_message_ = std::move(other.formatted_message_);
        }
        return *this;
    }

    const std::string &LogMessage::get_raw_message() const { return raw_message_; }
    LogLevel LogMessage::get_level() const { return level_; }
    auto LogMessage::get_timestamp() const { return timestamp_; }
    const std::string &LogMessage::get_formatted_message() const { return formatted_message_; }

    // Logger 实现
    Logger::Logger(LogMode mode) { (void)mode; }

    // SyncLogger 实现
    SyncLogger::SyncLogger() : Logger(LogMode::SYNC), log_level_(LogLevel::INFO) {}

    void SyncLogger::log(LogLevel level, const std::string &msg)
    {
        if (static_cast<int>(level) < static_cast<int>(log_level_))
        {
            return;
        }

        LogMessage log_msg(msg, level);
        const std::string &formatted = log_msg.get_formatted_message();

        for (auto &sink : sinks_)
        {
            sink->log(formatted, level);
        }
    }

    void SyncLogger::flush()
    {
        for (auto &sink : sinks_)
        {
            sink->flush();
        }
    }

    void SyncLogger::add_sink(std::shared_ptr<LogSink> sink)
    {
        sinks_.push_back(sink);
    }

    void SyncLogger::set_log_level(LogLevel level)
    {
        log_level_ = level;
    }

    // AsyncLogger 实现
    AsyncLogger::AsyncLogger()
        : Logger(LogMode::ASYNC),
          log_level_(LogLevel::INFO),
          stop_flag_(false),
          worker_thread_(&AsyncLogger::worker_thread, this) {}

    AsyncLogger::~AsyncLogger()
    {
        stop_flag_ = true;
        if (worker_thread_.joinable())
        {
            worker_thread_.join();
        }
    }

    void AsyncLogger::log(LogLevel level, const std::string &msg)
    {
        if (static_cast<int>(level) < static_cast<int>(log_level_))
        {
            return;
        }

        LogMessage log_msg(msg, level);
        log_queue_.enqueue(std::move(log_msg));
    }

    void AsyncLogger::flush()
    {
        // 等待队列清空
        while (!log_queue_.empty())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        std::lock_guard<std::mutex> lock(sinks_mutex_);
        for (auto &sink : sinks_)
        {
            sink->flush();
        }
    }

    void AsyncLogger::add_sink(std::shared_ptr<LogSink> sink)
    {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        sinks_.push_back(sink);
    }

    void AsyncLogger::set_log_level(LogLevel level)
    {
        log_level_ = level;
    }

    void AsyncLogger::worker_thread()
    {
        LogMessage msg;

        while (!stop_flag_)
        {
            // 批量处理日志
            std::vector<LogMessage> batch;
            batch.reserve(100); // 预分配空间

            // 尝试批量获取日志项
            while (batch.size() < 100 && log_queue_.dequeue(msg))
            {
                batch.push_back(std::move(msg));
            }

            if (!batch.empty())
            {
                std::lock_guard<std::mutex> lock(sinks_mutex_);
                for (auto &log_msg : batch)
                {
                    const std::string &formatted = log_msg.get_formatted_message();
                    for (auto &sink : sinks_)
                    {
                        sink->log(formatted, log_msg.get_level());
                    }
                }
            }
            else
            {
                // 如果队列为空，短暂休眠
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }

        // 处理剩余的日志项
        while (log_queue_.dequeue(msg))
        {
            const std::string &formatted = msg.get_formatted_message();
            std::lock_guard<std::mutex> lock(sinks_mutex_);
            for (auto &sink : sinks_)
            {
                sink->log(formatted, msg.get_level());
            }
        }
    }

    // LoggerManager 实现
    LoggerManager &LoggerManager::instance()
    {
        static LoggerManager instance;
        return instance;
    }

    void LoggerManager::set_logger(std::unique_ptr<Logger> logger)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        logger_ = std::move(logger);
    }

    Logger *LoggerManager::get_logger()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return logger_.get();
    }

    void LoggerManager::set_log_level(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logger_)
        {
            logger_->set_log_level(level);
        }
    }

    void LoggerManager::add_sink(std::shared_ptr<LogSink> sink)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logger_)
        {
            logger_->add_sink(sink);
        }
    }

    void LoggerManager::reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        logger_.reset();
    }

} // namespace base
