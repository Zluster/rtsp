
#include "Poller.hpp"
#include "EpollPoller.hpp"

namespace net
{

    Poller::Poller(EventLoop *loop) : loop_(loop), channels_()
    {
    }

    Poller::~Poller() = default;

    // 实现静态工厂方法
    Poller *Poller::newDefaultPoller(EventLoop *loop)
    {
        return new EpollPoller(loop);
    }

} // namespace net