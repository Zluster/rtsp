#pragma once
#include "EventLoop.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
namespace net
{
    class EventLoop;
    class EventLoopThread : base::Noncopyable
    {

    public:
        using ThreadInitCallback = std::function<void(EventLoop *)>;
        EventLoopThread(const ThreadInitCallback &initCallback, const std::string &name);
        EventLoopThread();
        ~EventLoopThread();

        EventLoop *getLoop() const { return loop_; }
        EventLoop *startLoop();
        void stopLoop();

    private:
        void threadFunc();
        EventLoop *loop_;
        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        ThreadInitCallback callback_;
        bool exiting_;
        std::string name_;
    };
}