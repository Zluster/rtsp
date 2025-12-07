#include "InetAddress.hpp"
#include "Logger.hpp"
#include <cstring>
namespace net
{
    InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
    {
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof(addr6_));
            addr6_.sin6_family = AF_INET6;
            addr6_.sin6_port = ::htons(port);
            addr6_.sin6_addr = loopbackOnly ? in6addr_loopback : in6addr_any;
        }
        else
        {
            memset(&addr4_, 0, sizeof(addr4_));
            addr4_.sin_family = AF_INET;
            addr4_.sin_port = ::htons(port);
            addr4_.sin_addr.s_addr = loopbackOnly ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
        }
    }

    InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6)
    {
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof(addr6_));
            addr6_.sin6_family = AF_INET6;
            addr6_.sin6_port = ::htons(port);

            if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0)
            {
                LOG_FATAL("InetAddress::InetAddress() invalid ipv6 address");
            }
        }
        else
        {
            memset(&addr4_, 0, sizeof(addr4_));
            addr4_.sin_family = AF_INET;
            addr4_.sin_port = ::htons(port);

            if (::inet_pton(AF_INET, ip.c_str(), &addr4_.sin_addr) <= 0)
            {
                LOG_FATAL("InetAddress::InetAddress() invalid ipv4 address");
            }
        }
    }

    std::string InetAddress::ip() const
    {
        char buf[INET6_ADDRSTRLEN];
        if (addr4_.sin_family == AF_INET)
        {
            ::inet_ntop(AF_INET, &addr4_.sin_addr, buf, INET_ADDRSTRLEN);
        }
        else if (addr6_.sin6_family == AF_INET6)
        {
            ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, INET6_ADDRSTRLEN);
        }
        else
        {
            LOG_FATAL("InetAddress::ip() invalid address family");
        }
        return buf;
    }

    std::string InetAddress::toIpPort() const
    {
        char buf[64] = {0};
        if (addr4_.sin_family == AF_INET)
        {
            ::inet_ntop(AF_INET, &addr4_.sin_addr, buf, INET_ADDRSTRLEN);
        }
        else if (addr6_.sin6_family == AF_INET6)
        {
            ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, INET6_ADDRSTRLEN);
        }
        size_t end = strlen(buf);
        uint16_t port = ntohs(addr4_.sin_family == AF_INET ? addr4_.sin_port : addr6_.sin6_port);
        snprintf(buf + end, sizeof(buf) - end, ":%u", port);
        return std::string(buf);
    }
    uint16_t InetAddress::toPort() const
    {
        if (addr4_.sin_family == AF_INET)
        {
            return ntohs(addr4_.sin_port);
        }
        else if (addr6_.sin6_family == AF_INET6)
        {
            return ntohs(addr6_.sin6_port);
        }
        return 0;
    }

    socklen_t InetAddress::getSockAddrLen() const
    {
        return addr4_.sin_family == AF_INET ? sizeof(addr4_) : sizeof(addr6_);
    }

    void InetAddress::setSockAddr(const sockaddr *addr, socklen_t addrlen)
    {
        if (addr4_.sin_family == AF_INET)
        {
            ::memcpy(&addr4_, addr, addrlen);
        }
        else if (addr6_.sin6_family == AF_INET6)
        {
            ::memcpy(&addr6_, addr, addrlen);
        }
    }
}