#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <cinttypes>

namespace base
{
    using TimerCallback = std::function<void()>;

    class Timestamp
    {
    public:
        static const int64_t kMicroSecondsPerSecond = 1000 * 1000;

        Timestamp() : microSecondsSinceEpoch_(0) {}
        explicit Timestamp(int64_t microSecondsSinceEpochArg)
            : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

        void swap(Timestamp &that)
        {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds = true) const;
        bool valid() const { return microSecondsSinceEpoch_ > 0; }
        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
        static Timestamp now();
        static Timestamp invalid() { return Timestamp(); }

    private:
        int64_t microSecondsSinceEpoch_;
    };

    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator>(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
    }

    inline bool operator<=(Timestamp lhs, Timestamp rhs)
    {
        return !(lhs > rhs);
    }

    inline bool operator>=(Timestamp lhs, Timestamp rhs)
    {
        return !(lhs < rhs);
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    inline Timestamp operator+(Timestamp lhs, double seconds)
    {
        return Timestamp(lhs.microSecondsSinceEpoch() + static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond));
    }
    inline double timeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }

    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }

    class TimerId
    {
    public:
        TimerId() : id_(-1), sequence_(0) {}
        explicit TimerId(int id, int sequence) : id_(id), sequence_(sequence) {}

        int getId() const { return id_; }
        int getSequence() const { return sequence_; }

        bool operator==(const TimerId &other) const { return id_ == other.id_ && sequence_ == other.sequence_; }
        bool operator!=(const TimerId &other) const { return !(*this == other); }
        bool operator<(const TimerId &other) const { return id_ < other.id_ || (id_ == other.id_ && sequence_ < other.sequence_); }
        bool isValid() const { return id_ != -1 || sequence_ != 0; }

    private:
        int id_;
        int sequence_;
    };

    // 特化 std::hash<TimerId>：使用公有接口
} // namespace base

namespace std // 注意这部分的 namespace 位置
{
    template <>
    struct hash<base::TimerId>
    {
        size_t operator()(const base::TimerId &id) const noexcept
        {
            size_t hash1 = hash<int>()(id.getId());
            size_t hash2 = hash<int>()(id.getSequence());
            return hash1 ^ (hash2 << 1);
        }
    };
} // namespace std
namespace base
{
    class Timer
    {
    public:
        Timer(TimerCallback cb, const Timestamp &when, double interval)
            : callback_(std::move(cb)), expiration_(when), interval_(interval), repeat_(interval > 0.0), timerId_(++nextId_, 0) {}

        void run();
        Timestamp expiration() const { return expiration_; }
        bool isRepeat() const { return repeat_; }
        double interval() const { return interval_; }
        TimerId timerId() const { return timerId_; }
        void restart(const Timestamp &now);
        bool valid() const { return expiration_.valid(); }

    private:
        TimerCallback callback_;
        Timestamp expiration_;
        double interval_;
        bool repeat_;
        TimerId timerId_;
        static int nextId_;
    };

    class TimerQueue
    {
    public:
        TimerQueue() = default; // 显式默认构造
        ~TimerQueue() = default;

        TimerId addTimer(TimerCallback cb, const Timestamp &when, double interval);
        void cancel(TimerId timerId);
        void handleExpiredTimers();

    private:
        using TimerPtr = std::shared_ptr<Timer>;

        struct TimerCompare
        {
            bool operator()(const TimerPtr &a, const TimerPtr &b) const
            {
                return a->expiration() > b->expiration();
            }
        };

        std::vector<TimerPtr> timers_;
        std::unordered_map<TimerId, TimerPtr> activeTimers_; // 现在可正常构造

        void addTimerInHeap(TimerPtr timer);
        void removeTimerFromHeap(TimerPtr timer);
        void handleExpiration(TimerPtr timer);
    };
}