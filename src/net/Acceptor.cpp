#include "Acceptor.hpp"
#include "Logger.hpp"
#include "EventLoop.hpp"
#include <errno.h>

namespace net
{
    Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
        : loop_(loop),
          acceptSocket_(),
          acceptChannel_(loop, acceptSocket_.fd()),
          listenning_(false)
    {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reusePort);
        acceptSocket_.bindAddress(listenAddr);
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
    }

    Acceptor::~Acceptor()
    {
        acceptChannel_.disableAll();
        acceptChannel_.remove();
    }

    void Acceptor::listen()
    {
        loop_->assertInLoopThread();
        listenning_ = true;
        acceptSocket_.listen();
        acceptChannel_.enableReading();
    }

    void Acceptor::handleRead()
    {
        loop_->assertInLoopThread();
        InetAddress peerAddr;
        int connfd = acceptSocket_.accept(&peerAddr);
        if (connfd >= 0)
        {
            if (newConnectionCallback_)
            {
                newConnectionCallback_(connfd, peerAddr);
            }
            else
            {
                ::close(connfd);
            }
        }
        else
        {
            LOG_ERROR("Acceptor::handleRead accept error: %d", errno);
        }
    }
}