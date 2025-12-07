#pragma once
#include <vector>
#include <sys/epoll.h>
#include "Poller.hpp"
#include "Timer.hpp"
namespace net
{

    class EventLoop;
    class Channel;
    class EpollPoller : public Poller
    {
    public:
        explicit EpollPoller(EventLoop *loop);
        ~EpollPoller();

        base::Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
        void updateChannel(Channel *channel) override;
        void removeChannel(Channel *channel) override;
        bool hasChannel(Channel *channel) const override;

    private:
        static const int kInitEventListSize = 16;
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
        void update(int operation, Channel *channel);

        using EventList = std::vector<struct epoll_event>;
        int epollfd_;
        EventList events_;
    };
} // namespace name
