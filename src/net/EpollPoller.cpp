#include "EpollPoller.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include <assert.h>
#include <errno.h>
#include <cstring>
#include <cassert>
namespace net
{
    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;

    EpollPoller::EpollPoller(EventLoop *loop)
        : Poller(loop),
          epollfd_(epoll_create1(EPOLL_CLOEXEC)),
          events_(kInitEventListSize)
    {
        assert(epollfd_ >= 0);
    }

    EpollPoller::~EpollPoller()
    {
        close(epollfd_);
    }

    base::Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
    {
        int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
        base::Timestamp now(base::Timestamp::now());

        if (numEvents > 0)
        {
            LOG_DEBUG("epoll_wait return %d events", numEvents);
            fillActiveChannels(numEvents, activeChannels);
            if (static_cast<size_t>(numEvents) == events_.size())
            {
                events_.resize(events_.size() * 2);
            }
        }
        else if (numEvents == 0)
        {
            LOG_DEBUG("epoll_wait timeout");
        }
        else
        {
            if (errno != EINTR)
            {
                LOG_ERROR("EpollPoller::poll() error: %s", strerror(errno));
            }
        }

        return now;
    }

    void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
    {
        for (int i = 0; i < numEvents; ++i)
        {
            Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            activeChannels->push_back(channel);
        }
    }

    void EpollPoller::update(int operation, Channel *channel)
    {
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = channel->events();
        event.data.ptr = channel;
        int fd = channel->fd();
        LOG_INFO("epoll_ctl op = %s fd = %d event = %04x",
                 operation == EPOLL_CTL_ADD ? "ADD" : operation == EPOLL_CTL_DEL ? "DEL"
                                                                                 : "MOD",
                 fd, event.events);
        if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
        {
            LOG_ERROR("epoll_ctl op = %s fd = %d event = %04x, errno = %s", operation == EPOLL_CTL_ADD ? "ADD" : operation == EPOLL_CTL_DEL ? "DEL"
                                                                                                                                            : "MOD",
                      fd, event.events, strerror(errno));
        }
    }
    void EpollPoller::updateChannel(Channel *channel)
    {
        const int index = channel->index();
        int fd = channel->fd();
        if (index == kNew || index == kDeleted)
        {
            if (index == kNew)
            {
                assert(channels_.find(fd) == channels_.end());
                channels_[fd] = channel;
            }
            else
            {
                assert(channels_.find(fd) != channels_.end());
                assert(channels_[fd] == channel);
            }
            channel->setIndex(kAdded);
            update(EPOLL_CTL_ADD, channel);
        }
        else
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
            assert(index == kAdded);
            if (channel->isNoneEvent())
            {
                update(EPOLL_CTL_DEL, channel);
                channel->setIndex(kDeleted);
            }
            else
            {
                update(EPOLL_CTL_MOD, channel);
            }
        }
    }
    void EpollPoller::removeChannel(Channel *channel)
    {
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->isNoneEvent());
        int index = channel->index();
        assert(index == kAdded || index == kDeleted);
        size_t n = channels_.erase(fd);
        assert(n == 1);
        if (index == kAdded)
        {
            update(EPOLL_CTL_DEL, channel);
        }
        channel->setIndex(kNew);
    }

    bool EpollPoller::hasChannel(Channel *channel) const
    {
        return channels_.find(channel->fd()) != channels_.end();
    }
}