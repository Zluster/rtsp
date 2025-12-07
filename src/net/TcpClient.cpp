#include "TcpClient.hpp"
#include "Connector.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "Logger.hpp"
#include <stdio.h>

namespace net
{
    TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &name)
        : loop_(loop),
          connector_(std::make_shared<Connector>(loop, serverAddr)),
          name_(name),
          connectionCallback_(TcpConnection::defaultConnectionCallback),
          messageCallback_(TcpConnection::defaultMessageCallback),
          retry_(false),
          connect_(true)
    {
        connector_->setNewConnectionCallback(
            std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
        LOG_INFO("TcpClient::TcpClient[%s] - connector %p", name_.c_str(), connector_.get());
    }

    TcpClient::~TcpClient()
    {
        LOG_INFO("TcpClient::~TcpClient[%s] - connector %p", name_.c_str(), connector_.get());
        TcpConnectionPtr conn;
        bool unique = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            unique = connection_.unique();
            conn = connection_;
        }

        if (conn)
        {
            assert(loop_ == conn->getLoop());
            // FIXME: not 100% safe, if we are in different thread
            CloseCallback cb = std::bind(&TcpClient::removeConnection, this, std::placeholders::_1);
            loop_->runInLoop(
                std::bind(&TcpConnection::setCloseCallback, conn, cb));

            if (unique)
            {
                conn->forceClose();
            }
        }
        else
        {
            connector_->stop();
        }
    }

    void TcpClient::connect()
    {
        LOG_INFO("TcpClient::connect[%s] - connecting to %s",
                 name_.c_str(), connector_->serverAddress().toIpPort().c_str());
        connect_ = true;
        connector_->start();
    }

    void TcpClient::disconnect()
    {
        connect_ = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (connection_)
            {
                connection_->shutdown();
            }
        }
    }

    void TcpClient::stop()
    {
        connect_ = false;
        connector_->stop();
    }

    void TcpClient::newConnection(int sockfd)
    {
        loop_->assertInLoopThread();
        InetAddress peerAddr(Socket::getPeerAddr(sockfd));
        char buf[32];
        snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
        ++nextConnId_;
        std::string connName = name_ + buf;

        InetAddress localAddr(Socket::getLocalAddr(sockfd));
        // FIXME poll with zero timeout to double confirm the new connection
        TcpConnectionPtr conn(new TcpConnection(loop_,
                                                connName,
                                                sockfd,
                                                localAddr,
                                                peerAddr));

        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connection_ = conn;
        }
        conn->connectEstablished();
    }

    void TcpClient::removeConnection(const TcpConnectionPtr &conn)
    {
        loop_->assertInLoopThread();
        assert(loop_ == conn->getLoop());

        {
            std::lock_guard<std::mutex> lock(mutex_);
            assert(connection_ == conn);
            connection_.reset();
        }

        loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        if (retry_ && connect_)
        {
            LOG_INFO("TcpClient::connect[%s] - Reconnecting to %s",
                     name_.c_str(), connector_->serverAddress().toIpPort().c_str());
            connector_->restart();
        }
    }

    // 静态成员初始化
    int TcpClient::nextConnId_ = 1;
} // namespace net