#include "Timer.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <memory>
#include <algorithm>

namespace base
{

    // ===== Timestamp 实现 =====
    Timestamp Timestamp::now()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        int64_t seconds = tv.tv_sec;
        return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
    }

    std::string Timestamp::toString() const
    {
        char buf[32] = {0};
        int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
        int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
        snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
        return buf;
    }

    std::string Timestamp::toFormattedString(bool showMicroseconds) const
    {
        char buf[64] = {0};
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        struct tm tm_time;
        gmtime_r(&seconds, &tm_time);

        if (showMicroseconds)
        {
            int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
            snprintf(buf, sizeof(buf) - 1, "%4d%02d%02d %02d:%02d:%02d.%06d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                     microseconds);
        }
        else
        {
            snprintf(buf, sizeof(buf) - 1, "%4d%02d%02d %02d:%02d:%02d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }
        return buf;
    }

    // ===== Timer 实现 =====
    void Timer::run()
    {
        if (valid())
        {
            callback_();
        }
    }

    void Timer::restart(const Timestamp &now)
    {
        if (repeat_)
        {
            expiration_ = addTime(now, interval_);
        }
        else
        {
            expiration_ = Timestamp::invalid(); // 非重复定时器标记为无效
        }
    }

    // ===== TimerQueue 实现 =====
    TimerId TimerQueue::addTimer(TimerCallback cb, const Timestamp &when, double interval)
    {
        TimerPtr timer = std::make_shared<Timer>(cb, when, interval);
        TimerId timerId = timer->timerId();

        addTimerInHeap(timer);
        activeTimers_[timerId] = timer;

        return timerId;
    }

    void TimerQueue::cancel(TimerId timerId)
    {
        auto it = activeTimers_.find(timerId);
        if (it != activeTimers_.end())
        {
            auto timer = it->second;
            removeTimerFromHeap(timer);
            activeTimers_.erase(it);
        }
    }

    void TimerQueue::handleExpiredTimers()
    {
        Timestamp now = Timestamp::now();

        while (!timers_.empty())
        {
            TimerPtr top = timers_.front();
            if (top->expiration() > now)
                break;

            std::pop_heap(timers_.begin(), timers_.end(), TimerCompare());
            timers_.pop_back();

            handleExpiration(top);
        }
    }

    void TimerQueue::addTimerInHeap(TimerPtr timer)
    {
        timers_.push_back(timer);
        std::push_heap(timers_.begin(), timers_.end(), TimerCompare());
    }

    void TimerQueue::removeTimerFromHeap(TimerPtr timer)
    {
        auto it = std::find(timers_.begin(), timers_.end(), timer);
        if (it != timers_.end())
        {
            *it = timers_.back();
            timers_.pop_back();

            if (it != timers_.end())
            {
                std::push_heap(timers_.begin(), it + 1, TimerCompare());
                std::pop_heap(timers_.begin(), it + 1, TimerCompare());
            }
        }
    }

    void TimerQueue::handleExpiration(TimerPtr timer)
    {
        timer->run();

        if (timer->isRepeat())
        {
            timer->restart(Timestamp::now());
            addTimerInHeap(timer);
        }
        else
        {
            activeTimers_.erase(timer->timerId());
        }
    }

    int Timer::nextId_ = 0;
} // namespace base