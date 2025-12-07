#include "Channel.hpp"
#include "Logger.hpp"
#include "EventLoop.hpp"

#include <sstream>
#include <poll.h>
#include <assert.h>
#include <sys/epoll.h>

namespace net
{
    const int Channel::kNoneEvent = 0;
    const int Channel::kReadEvent = POLLIN | POLLPRI;
    const int Channel::kWriteEvent = POLLOUT;

    Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop),
          fd_(fd),
          events_(0),
          revents_(0),
          index_(-1),
          addedToLoop_(false),
          eventHandling_(false),
          readCallback_(nullptr),
          writeCallback_(nullptr),
          closeCallback_(nullptr),
          errorCallback_(nullptr) {}

    Channel::~Channel()
    {
        assert(!eventHandling_);
        assert(!addedToLoop_);
        if (loop_->isInLoopThread())
        {
            assert(!loop_->hasChannel(this));
        }
    }

    void Channel::update()
    {
        addedToLoop_ = true;
        loop_->updateChannel(this);
    }

    void Channel::remove()
    {
        addedToLoop_ = false;
        loop_->removeChannel(this);
    }

    void Channel::handleEvent(Timestamp receiveTime)
    {
        eventHandling_ = true;
        LOG_WARN("%s", eventsToString(fd_, revents_).c_str());
        if (revents_ & POLLHUP && !(revents_ & POLLIN))
        {
            if (closeCallback_)
            {
                closeCallback_();
            }
        }
        else if (revents_ & POLLERR)
        {
            if (errorCallback_)
            {
                errorCallback_();
            }
        }
        else if (revents_ & (POLLIN | POLLPRI))
        {
            if (readCallback_)
            {
                readCallback_(receiveTime);
            }
        }
        else if (revents_ & POLLOUT)
        {
            if (writeCallback_)
            {
                writeCallback_();
            }
        }
        eventHandling_ = false;
    }
    std::string eventsToString(int fd, int ev)
    {
        std::ostringstream oss;
        oss << fd << ": ";
        if (ev & POLLIN)
            oss << "IN ";
        if (ev & POLLPRI)
            oss << "PRI ";
        if (ev & POLLOUT)
            oss << "OUT ";
        if (ev & POLLHUP)
            oss << "HUP ";
        if (ev & POLLRDHUP)
            oss << "RDHUP ";
        if (ev & POLLERR)
            oss << "ERR ";
        if (ev & POLLNVAL)
            oss << "NVAL ";

        return oss.str();
    }
}