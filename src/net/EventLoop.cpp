#include "Logger.hpp"
#include "EventLoop.hpp"
#include "Poller.hpp"
#include <cassert>
#include <sys/eventfd.h>
namespace net
{
    namespace
    {
        const int kPollTimeMs = 10000;
    }

    EventLoop::EventLoop()
        : looping_(false),
          quit_(false),
          threadId_(std::this_thread::get_id()),
          pollReturnTime_(Timestamp::now()),
          poller_(Poller::newDefaultPoller(this)),
          timerQueue_(new base::TimerQueue()),
          wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
          wakeUpChannel_(new Channel(this, wakeupFd_)),
          callingPendingFunctors_(false)
    {
        if (wakeupFd_ < 0)
        {
            LOG_FATAL("eventfd creation EventLoop::EventLoop");
        }
        LOG_DEBUG("EventLoop created %p", this);
        wakeUpChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
        wakeUpChannel_->enableReading();
    }

    EventLoop::~EventLoop()
    {
        assert(!looping_);
        assert(!callingPendingFunctors_);
        LOG_DEBUG("EventLoop %p destructs in thread %p", this, std::this_thread::get_id());
        wakeUpChannel_->disableAll();
        wakeUpChannel_->remove();
        ::close(wakeupFd_);
    }

    void EventLoop::loop()
    {
        assert(!looping_);
        assertInLoopThread();
        looping_ = true;
        quit_ = false;
        LOG_INFO("EventLoop %p start looping in thread %d", this, gettid());
        while (!quit_)
        {
            activeChannels_.clear();
            pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
            for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
            {
                (*it)->handleEvent(pollReturnTime_);
            }
            doPendingFunctors();
        }
        LOG_INFO("EventLoop %p stop looping", this);
        looping_ = false;
    }

    void EventLoop::quit()
    {
        quit_ = true;
        if (!isInLoopThread())
        {
            wakeup();
        }
    }

    void EventLoop::runInLoop(const Functor &cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(cb);
        }
    }

    void EventLoop::queueInLoop(const Functor &cb)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.push_back(cb);
        }
        if (!isInLoopThread() || callingPendingFunctors_)
        {
            wakeup();
        }
    }

    TimerId EventLoop::runAt(const Timestamp &time, TimerCallback cb)
    {
        return timerQueue_->addTimer(cb, time, 0.0);
    }

    TimerId EventLoop::runAfter(double delay, TimerCallback cb)
    {
        Timestamp time(addTime(Timestamp::now(), delay));
        return runAt(time, cb);
    }

    TimerId EventLoop::runEvery(double interval, TimerCallback cb)
    {
        Timestamp time(addTime(Timestamp::now(), interval));
        return timerQueue_->addTimer(cb, time, interval);
    }

    void EventLoop::cancel(TimerId timerId)
    {
        return timerQueue_->cancel(timerId);
    }

    void EventLoop::updateChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        poller_->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        poller_->removeChannel(channel);
    }

    bool EventLoop::hasChannel(Channel *channel) const
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        return poller_->hasChannel(channel);
    }

    void EventLoop::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
        if (n != sizeof one)
        {
            LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
        }
    }

    void EventLoop::handleRead()
    {
        uint64_t one = 1;
        ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
        if (n != sizeof one)
        {
            LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
        }
    }

    void EventLoop::doPendingFunctors()
    {
        std::vector<Functor> functors;
        callingPendingFunctors_ = true;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            functors.swap(pendingFunctors_);
        }
        for (const Functor &functor : functors)
        {
            functor();
        }
        callingPendingFunctors_ = false;
    }
    void EventLoop::assertInLoopThread() const
    {
        if (!isInLoopThread())
        {
            LOG_FATAL("EventLoop::assertInLoopThread - EventLoop %p was created in thread %d, current thread is %d",
                      this, gettid(), gettid());
        }
    }
}