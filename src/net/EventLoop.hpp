#pragma once
#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include "Timer.hpp"
#include "Noncopyable.hpp"
#include "Channel.hpp"
using namespace base;
namespace net
{
    class Channel;
    class Poller;
    class TimerQueue;

    class EventLoop : base::Noncopyable
    {
    public:
        using Functor = std::function<void()>;

        EventLoop();
        ~EventLoop();

        void loop();
        void quit();
        void wakeup();
        Timestamp pollReturnTime() const { return pollReturnTime_; }

        void runInLoop(const Functor &cb);
        void queueInLoop(const Functor &cb);

        TimerId runAt(const Timestamp &time, TimerCallback cb);
        TimerId runAfter(double delay, TimerCallback cb);
        TimerId runEvery(double interval, TimerCallback cb);
        void cancel(TimerId timerId);

        void updateChannel(Channel *channel);
        void removeChannel(Channel *channel);
        bool hasChannel(Channel *channel) const;

        void assertInLoopThread() const;

        bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }

    private:
        void handleRead();
        void doPendingFunctors();
        using ChannelList = std::vector<Channel *>;

        std::atomic<bool> looping_;
        std::atomic<bool> quit_;
        const std::thread::id threadId_;
        Timestamp pollReturnTime_;
        std::unique_ptr<Poller> poller_;
        std::unique_ptr<base::TimerQueue> timerQueue_;
        ChannelList activeChannels_;

        int wakeupFd_;
        std::unique_ptr<Channel> wakeUpChannel_;
        std::mutex mutex_;
        std::vector<Functor> pendingFunctors_;
        std::atomic<bool> callingPendingFunctors_;
    };
}