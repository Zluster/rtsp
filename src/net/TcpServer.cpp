#include "TcpConnection.hpp"
#include "Logger.hpp"
#include "TcpServer.hpp"
#include "EventLoopThreadPool.hpp"
#include "Acceptor.hpp"
#include <assert.h>
namespace net
{

    TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, bool reusePort)
        : loop_(loop),
          ipPort_(listenAddr.toIpPort()),
          name_(name),
          acceptor_(new Acceptor(loop, listenAddr, reusePort)),
          threadPool_(new EventLoopThreadPool(loop, name)),
          connectionCallback_(nullptr),
          messageCallback_(nullptr),
          writeCompleteCallback_(nullptr),
          highWaterMarkCallback_(nullptr),
          highWaterMark_(10 * 1024 * 1024),
          started_(false),
          nextConnId_(1)
    {
        acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
    }

    TcpServer::~TcpServer()
    {
        LOG_DEBUG("TcpServer:~TcpServer[%s] destructing", name_.c_str());
        loop_->assertInLoopThread();

        for (auto &item : connections_)
        {
            TcpConnectionPtr conn(item.second);
            item.second.reset();
            conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        }
    }

    void TcpServer::setThreadNum(int Threads)
    {
        threadPool_->setThreadNum(Threads);
    }

    void TcpServer::start()
    {
        if (!started_.exchange(true))
        {
            threadPool_->start();
            assert(!acceptor_->listenning());
            loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        }
    }
    void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
    {
        loop_->assertInLoopThread();
        EventLoop *ioLoop = threadPool_->getNextLoop();
        char buf[64];
        snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
        ++nextConnId_;
        std::string connName = name_ + buf;

        LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
                 name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

        InetAddress localAddr(Socket::getLocalAddr(sockfd));
        TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                                connName,
                                                sockfd,
                                                localAddr,
                                                peerAddr));

        connections_[connName] = conn;
        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setHighWaterMarkCallback(highWaterMarkCallback_, highWaterMark_);

        conn->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

        ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    }
    void TcpServer::removeConnection(const TcpConnectionPtr &conn)
    {
        loop_->runInLoop(
            std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }

    void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
    {
        loop_->assertInLoopThread();
        LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s",
                 name_.c_str(), conn->getName().c_str());

        size_t n = connections_.erase(conn->getName());
        (void)n;
        assert(n == 1);

        EventLoop *ioLoop = conn->getLoop();
        ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }

}