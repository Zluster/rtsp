#pragma once

#include <map>
#include <vector>
#include "Timer.hpp"
#include "Noncopyable.hpp"

namespace net
{
    class EventLoop;
    class Channel;
    class EpollPoller;
    class Poller : base::Noncopyable
    {
    public:
        using ChannelList = std::vector<Channel *>;
        explicit Poller(EventLoop *loop);

        ~Poller();

        virtual base::Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;
        virtual bool hasChannel(Channel *channel) const = 0;

        static Poller *newDefaultPoller(EventLoop *loop);

    protected:
        using ChannelMap = std::map<int, Channel *>;
        EventLoop *loop_;
        ChannelMap channels_;
    };
}