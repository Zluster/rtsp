#pragma once
#include <string>
#include <vector>
#include <memory>
#include "EventLoopThread.hpp"
#include "Noncopyable.hpp"

namespace net
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool
    {
    public:
        typedef std::function<void(EventLoop *)> ThreadInitCallback;

        EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
        ~EventLoopThreadPool();

        void setThreadNum(int numThreads) { numThreads_ = numThreads; }
        void start(const ThreadInitCallback &cb = ThreadInitCallback());

        EventLoop *getNextLoop();
        EventLoop *getLoopForHash(size_t hashCode);
        std::vector<EventLoop *> getAllLoops();

    private:
        EventLoop *baseLoop_;
        std::string name_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop *> loops_;
    };
}