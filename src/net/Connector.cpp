#include "Connector.hpp"
#include "EventLoop.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>

namespace net
{
    const int Connector::kMaxRetryDelayMs;
    const int Connector::kInitRetryDelayMs;

    Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop),
          serverAddr_(serverAddr),
          connect_(false),
          state_(kDisconnected),
          retryDelayMs_(kInitRetryDelayMs)
    {
        LOG_DEBUG("Connector::Connector()");
    }

    Connector::~Connector()
    {
        LOG_DEBUG("Connector::~Connector()");
        loop_->assertInLoopThread();
        assert(state_ == kDisconnected);
    }

    void Connector::start()
    {
        connect_ = true;
        loop_->runInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
    }

    void Connector::startInLoop()
    {
        loop_->assertInLoopThread();
        assert(state_ == kDisconnected);
        if (connect_)
        {
            connect();
        }
        else
        {
            LOG_DEBUG("do not connect");
        }
    }

    void Connector::stop()
    {
        connect_ = false;
        loop_->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
    }

    void Connector::stopInLoop()
    {
        loop_->assertInLoopThread();
        if (state_ == kConnecting)
        {
            setState(kDisconnected);
            int sockfd = removeAndResetChannel();
            retry(sockfd);
        }
    }

    void Connector::connect()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sockfd < 0)
        {
            LOG_ERROR("create socket failed in Connector::connect");
            return;
        }
        int ret = ::connect(sockfd, serverAddr_.getSockAddr(), serverAddr_.getSockAddrLen());
        int savedErrno = (ret == 0) ? 0 : errno;
        switch (savedErrno)
        {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR("connect error in Connector::startInLoop: %d", savedErrno);
            ::close(sockfd);
            break;

        default:
            LOG_ERROR("Unexpected error in Connector::startInLoop: %d", savedErrno);
            ::close(sockfd);
            // connectErrorCallback_();
            break;
        }
    }

    void Connector::connecting(int sockfd)
    {
        setState(kConnecting);
        assert(!channel_);
        channel_.reset(new Channel(loop_, sockfd));
        channel_->setWriteCallback(
            std::bind(&Connector::handleWrite, shared_from_this()));
        channel_->setErrorCallback(
            std::bind(&Connector::handleError, shared_from_this()));

        channel_->enableWriting();
    }

    int Connector::removeAndResetChannel()
    {
        channel_->disableAll();
        channel_->remove();
        int sockfd = channel_->fd();
        // Can't reset channel_ here, because we are inside Channel::handleEvent
        loop_->queueInLoop(std::bind(&Connector::resetChannel, shared_from_this()));
        return sockfd;
    }

    void Connector::resetChannel()
    {
        channel_.reset();
    }

    void Connector::handleWrite()
    {
        LOG_DEBUG("Connector::handleWrite");

        if (state_ != kConnecting)
        {
            return;
        }

        int sockfd = removeAndResetChannel();
        int err = 0;
        socklen_t optlen = sizeof(err);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &optlen);
        if (err)
        {
            LOG_WARN("Connector::handleWrite - SO_ERROR = %d", err);
            retry(sockfd);
        }
        else if (0)
        {
            LOG_WARN("Connector::handleWrite - Self connect");
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }

    void Connector::handleError()
    {
        LOG_ERROR("Connector::handleError state=%d", state_);
        if (state_ != kConnecting)
        {
            return;
        }

        int sockfd = removeAndResetChannel();
        int err = 0;
        socklen_t optlen = sizeof(err);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &optlen);
        LOG_DEBUG("SO_ERROR = %d", err);
        retry(sockfd);
    }

    void Connector::restart()
    {
        state_ = kDisconnected;
        if (connect_)
        {
            LOG_DEBUG("Connector::retry - Retry connecting to %s in %d milliseconds. ", serverAddr_.toIpPort().c_str(),
                      retryDelayMs_);
            loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::connect, shared_from_this()));
            retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
        }
    }
    void Connector::retry(int sockfd)
    {
        ::close(sockfd);
        setState(kDisconnected);
        if (connect_)
        {
            LOG_INFO("Connector::retry - Retry connecting to %s in %d ms.",
                     serverAddr_.toIpPort().c_str(), retryDelayMs_);
            loop_->runAfter(retryDelayMs_ / 1000.0,
                            std::bind(&Connector::startInLoop, shared_from_this()));
            retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
        }
        else
        {
            LOG_DEBUG("do not retry");
        }
    }
} // namespace net