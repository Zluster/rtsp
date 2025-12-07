#pragma once
#include "InetAddress.hpp"
#include "Noncopyable.hpp"
namespace net
{
    class InetAddress;
    class Socket : base::Noncopyable
    {
    public:
        Socket();
        explicit Socket(int sockfd)
            : sockfd_(sockfd)
        {
        }

        ~Socket();

        int fd() const { return sockfd_; }

        void bindAddress(const InetAddress &addr);

        void listen();

        int accept(InetAddress *peeraddr);

        void shutdownWrite();

        void setTcpNoDelay(bool on);

        void setReuseAddr(bool on);

        void setReusePort(bool on);

        void setKeepAlive(bool on);

        static InetAddress getLocalAddr(int sockfd);
        static InetAddress getPeerAddr(int sockfd);

    private:
        int sockfd_;
    };
}