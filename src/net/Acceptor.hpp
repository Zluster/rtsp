#pragma once
#include "Noncopyable.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "InetAddress.hpp"
#include <functional>

namespace net
{
    class EventLoop;
    class Acceptor : base::Noncopyable
    {
    public:
        using NewConnectionCallback = std::function<void(int, const InetAddress &)>;

        Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }

        bool listenning() const { return listenning_; }
        void listen();

    private:
        void handleRead();

        EventLoop *loop_;
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listenning_;
    };
}