#include "Socket.hpp"
#include "Logger.hpp"
#include <stdexcept>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
namespace net
{
    Socket::Socket()
    {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ == -1)
            throw std::runtime_error("Socket creation failed");
    }

    Socket::~Socket()
    {
        close(sockfd_);
    }

    void Socket::bindAddress(const InetAddress &localaddr)
    {
        if (bind(sockfd_, localaddr.getSockAddr(), sizeof(sockaddr_in)) == -1)
            throw std::runtime_error("Bind failed");
    }

    void Socket::listen()
    {
        if (::listen(sockfd_, SOMAXCONN) == -1)
            throw std::runtime_error("Listen failed");
    }

    int Socket::accept(InetAddress *peeraddr)
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
        if (connfd == -1)
            throw std::runtime_error("Accept failed");
        peeraddr->setSockAddr((const sockaddr *)&addr, len);
        return connfd;
    }
    void Socket::shutdownWrite()
    {
        if (::shutdown(sockfd_, SHUT_WR) == -1)
            throw std::runtime_error("Shutdown failed");
    }

    void Socket::setTcpNoDelay(bool on)
    {
        int optval = on ? 1 : 0;
        if (::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1)
            throw std::runtime_error("Setsockopt failed");
    }

    void Socket::setReuseAddr(bool on)
    {
        int optval = on ? 1 : 0;
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
            throw std::runtime_error("Setsockopt failed");
    }

    void Socket::setReusePort(bool on)
    {
        int optval = on ? 1 : 0;
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
            throw std::runtime_error("Setsockopt failed");
    }

    void Socket::setKeepAlive(bool on)
    {
        int optval = on ? 1 : 0;
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1)
            throw std::runtime_error("Setsockopt failed");
    }
    // src/net/Socket.cpp
    InetAddress Socket::getPeerAddr(int sockfd)
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getpeername(sockfd, (sockaddr *)&addr, &len) == -1)
        {
            LOG_ERROR("getpeername failed");
        }
        return InetAddress(addr);
    }

    InetAddress Socket::getLocalAddr(int sockfd)
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getsockname(sockfd, (sockaddr *)&addr, &len) == -1)
        {
            LOG_ERROR("getsockname failed");
        }
        return InetAddress(addr);
    }
}