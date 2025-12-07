#pragma once
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
namespace net
{
    class InetAddress
    {
    public:
        InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
        InetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

        explicit InetAddress(const struct sockaddr_in &addr) : addr4_(addr) {}
        explicit InetAddress(const struct sockaddr_in6 &addr) : addr6_(addr) {}

        std::string ip() const;
        std::string toIpPort() const;
        uint16_t toPort() const;

        const struct sockaddr *getSockAddr() const { return reinterpret_cast<const struct sockaddr *>(&addr6_); }
        socklen_t getSockAddrLen() const;
        void setSockAddr(const struct sockaddr *addr, socklen_t len);

    private:
        union
        {
            struct sockaddr_in addr4_;
            struct sockaddr_in6 addr6_;
        };
    };
}