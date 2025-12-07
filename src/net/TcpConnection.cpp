#include "TcpConnection.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <netinet/tcp.h>
#include <functional>
#include <assert.h>

namespace net
{
    TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                                 const InetAddress &localAddr, const InetAddress &peerAddr) : loop_(loop),                                                                                      // 1. 匹配声明顺序
                                                                                              name_(name),                                                                                      // 2.
                                                                                              state_(kConnecting),                                                                              // 3.
                                                                                              sockfd_(sockfd),                                                                                  // 4.
                                                                                              channel_(new Channel(loop, sockfd)),                                                              // 5.
                                                                                              localAddr_(localAddr),                                                                            // 6.
                                                                                              peerAddr_(peerAddr),                                                                              // 7.
                                                                                              connectionCallback_(std::bind(&TcpConnection::defaultConnectionCallback, std::placeholders::_1)), // 8.
                                                                                              messageCallback_(),                                                                               // 9.（若未初始化可留空，或按实际需求赋值）
                                                                                              writeCompleteCallback_(),                                                                         // 10.
                                                                                              closeCallback_(std::bind(&TcpConnection::defaultCloseCallback, std::placeholders::_1)),           // 11.
                                                                                              highWaterMarkCallback_(),                                                                         // 12.
                                                                                              HighWaterMark_(10 * 1024 * 1024)                                                                  // 13.
    {
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
        LOG_DEBUG("TcpConnection::ctor[%s] at %p fd=%d",
                  name_.c_str(), this, sockfd_);
    }

    TcpConnection::~TcpConnection()
    {
        LOG_DEBUG("TcpConnection::dtor[%s] at %p fd=%d state=%d",
                  name_.c_str(), this, sockfd_, static_cast<int>(state_.load()));
        assert(state_ == kDisconnected);
        //        loop_->removeChannel(channel_.get());
        close(sockfd_);
    }

    void TcpConnection::send(const std::string &message)
    {
        if (state_.load() == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(message);
            }
            else
            {
                loop_->runInLoop(
                    std::bind(static_cast<void (TcpConnection::*)(const std::string &)>(&TcpConnection::sendInLoop),
                              this,
                              message));
            }
        }
    }

    void TcpConnection::send(Buffer *buf)
    {
        if (state_.load() == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(buf->retrieveAllAsString());
            }
            else
            {
                loop_->runInLoop(std::bind(static_cast<void (TcpConnection::*)(const std::string &)>(&TcpConnection::sendInLoop),
                                           this,
                                           buf->retrieveAllAsString()));
            }
        }
    }

    void TcpConnection::send(const void *data, size_t len)
    {
        if (state_.load() == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(data, len);
            }
            else
            {
                loop_->runInLoop(
                    std::bind(static_cast<void (TcpConnection::*)(const std::string &)>(&TcpConnection::sendInLoop),
                              this,
                              std::string(static_cast<const char *>(data), len)));
            }
        }
    }

    void TcpConnection::sendInLoop(const std::string &message)
    {
        sendInLoop(message.data(), message.size());
    }

    void TcpConnection::sendInLoop(Buffer *buf)
    {
        sendInLoop(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
    }
    void TcpConnection::sendInLoop(const void *data, size_t len)
    {
        loop_->assertInLoopThread();
        ssize_t nwrote = 0;
        size_t remaining = len;
        bool faultError = false;

        if (state_.load() == kDisconnected)
        {
            LOG_WARN("disconnected, give up writing");
            return;
        }

        if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
        {
            nwrote = ::write(sockfd_, data, len);
            if (nwrote >= 0)
            {
                remaining = len - nwrote;
                if (remaining == 0 && writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            else
            {
                nwrote = 0;
                if (errno != EWOULDBLOCK)
                {
                    LOG_ERROR("TcpConnection::sendInLoop");
                    if (errno == EPIPE || errno == ECONNRESET)
                    {
                        faultError = true;
                    }
                }
            }
        }

        if (!faultError && remaining > 0)
        {
            size_t oldLen = outputBuffer_.readableBytes();
            if (oldLen + remaining >= HighWaterMark_)
            {
                if (highWaterMarkCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(highWaterMarkCallback_,
                                  shared_from_this(),
                                  oldLen + remaining));
                }
            }
            outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
            if (!channel_->isWriting())
            {
                channel_->enableWriting();
            }
        }
    }

    void TcpConnection::shutdown()
    {
        if (state_.load() == kConnected)
        {
            setState(kDisconnecting);
            loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
        }
    }

    void TcpConnection::shutdownInLoop()
    {
        loop_->assertInLoopThread();
        if (!channel_->isWriting())
        {
            ::shutdown(sockfd_, SHUT_WR);
        }
    }

    void TcpConnection::setTcpNoDelay(bool on)
    {
        ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    }

    void TcpConnection::connectEstablished()
    {
        loop_->assertInLoopThread();
        setState(kConnected);
        channel_->enableReading();
        connectionCallback_(shared_from_this());
    }

    void TcpConnection::connectDestroyed()
    {
        loop_->assertInLoopThread();
        if (state_.load() == kConnected)
        {
            setState(kDisconnected);
            channel_->disableAll();
            connectionCallback_(shared_from_this());
        }
        channel_->remove();
    }

    void TcpConnection::handleRead(Timestamp receiveTime)
    {
        loop_->assertInLoopThread();
        int savedErrno = 0;
        ssize_t n = inputBuffer_.readFd(sockfd_, &savedErrno);
        if (n > 0)
        {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
        else if (n == 0)
        {
            handleClose();
        }
        else
        {
            errno = savedErrno;
            LOG_ERROR("TcpConnection::handleRead");
            handleError();
        }
    }

    void TcpConnection::handleWrite()
    {
        loop_->assertInLoopThread();
        if (channel_->isWriting())
        {
            ssize_t n = ::write(sockfd_,
                                outputBuffer_.peek(),
                                outputBuffer_.readableBytes());
            if (n > 0)
            {
                outputBuffer_.retrieve(n);
                if (outputBuffer_.readableBytes() == 0)
                {
                    channel_->disableWriting();
                    if (writeCompleteCallback_)
                    {
                        loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                    if (state_.load() == kDisconnecting)
                    {
                        shutdownInLoop();
                    }
                }
                else
                {
                    LOG_DEBUG("I am going to write more data");
                }
            }
            else
            {
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
        else
        {
            LOG_DEBUG("Connection fd = %d is down, no more writing", sockfd_);
        }
    }

    void TcpConnection::handleClose()
    {
        loop_->assertInLoopThread();
        LOG_DEBUG("fd = %d state = %d", sockfd_, static_cast<int>(state_.load()));
        assert(state_.load() == kConnected || state_.load() == kDisconnecting);

        channel_->disableAll();
        closeCallback_(shared_from_this());
    }

    void TcpConnection::handleError()
    {
        LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d", name_.c_str(), errno);
    }

    void TcpConnection::forceClose()
    {
        loop_->runInLoop(std::bind(&TcpConnection::forceCloseInLoop, this));
    }

    void TcpConnection::forceCloseInLoop()
    {
        loop_->assertInLoopThread();
        if (state_ == kConnected || state_ == kDisconnecting)
        {
            handleClose(); // 触发关闭逻辑
        }
    }
    void TcpConnection::defaultConnectionCallback(const TcpConnectionPtr &conn)
    {
        LOG_DEBUG("%s -> %s is %s",
                  conn->getLocalAddr().toIpPort().c_str(),
                  conn->getPeerAddr().toIpPort().c_str(),
                  (conn->connected() ? "UP" : "DOWN"));
    }
    void TcpConnection::defaultMessageCallback(const TcpConnectionPtr &conn,
                                               Buffer *buf,
                                               Timestamp receiveTime)
    {
        buf->retrieveAll();
        LOG_DEBUG("%s -> %s at %s",
                  conn->getLocalAddr().toIpPort().c_str(),
                  conn->getPeerAddr().toIpPort().c_str(),
                  receiveTime.toFormattedString().c_str());
    }
    void TcpConnection::defaultCloseCallback(const TcpConnectionPtr &conn)
    {
        //  conn->getLoop()->quit();
        // buf->retrieveAll();
        LOG_DEBUG("%s -> %s is closed",
                  conn->getLocalAddr().toIpPort().c_str(),
                  conn->getPeerAddr().toIpPort().c_str());
    }
}