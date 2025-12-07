#include "EventLoopThread.hpp"
#include "Logger.hpp"
#include <functional>
namespace net
{
    EventLoopThread::EventLoopThread(const ThreadInitCallback &initCallback, const std::string &name)
        : loop_(nullptr),

          thread_(std::bind(&EventLoopThread::threadFunc, this)),
          callback_(initCallback), // 初始化回调
          exiting_(false),
          name_(name) // 线程名称
    {
    }
    EventLoopThread::EventLoopThread() : EventLoopThread(ThreadInitCallback(), "EventLoopThread")
    {
    }
    EventLoopThread::~EventLoopThread()
    {
        stopLoop();
    }

    EventLoop *EventLoopThread::startLoop()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (loop_ == nullptr)
            {
                cond_.wait(lock);
            }
        }
        return loop_;
    }

    void EventLoopThread::stopLoop()
    {
        exiting_ = true;
        if (loop_ != nullptr)
        {
            loop_->quit();
        }
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    void EventLoopThread::threadFunc()
    {
        EventLoop loop;
        if (callback_)
        {
            callback_(&loop);
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = &loop;
            cond_.notify_one();
        }
        loop.loop();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = nullptr;
        }
    }

}