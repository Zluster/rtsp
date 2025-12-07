#include <gtest/gtest.h>
#include "net/InetAddress.hpp"
#include <cstring>

using namespace net;

// 测试IPv4地址初始化
TEST(InetAddressTest, IPv4Initialization)
{
    InetAddress addr(8080, false, false);
    EXPECT_EQ(addr.toPort(), 8080);
    EXPECT_EQ(addr.ip(), "0.0.0.0");

    InetAddress loopback(8888, true, false);
    EXPECT_EQ(loopback.ip(), "127.0.0.1");
}

// 测试IPv6地址初始化
TEST(InetAddressTest, IPv6Initialization)
{
    InetAddress addr(8080, false, true);
    EXPECT_EQ(addr.toPort(), 8080);
    EXPECT_EQ(addr.ip(), "::");

    InetAddress loopback(8888, true, true);
    EXPECT_EQ(loopback.ip(), "::1");
}

// 测试IP和端口字符串转换
TEST(InetAddressTest, IpPortConversion)
{
    InetAddress addr("192.168.1.1", 9090, false);
    EXPECT_EQ(addr.toIpPort(), "192.168.1.1:9090");
    EXPECT_EQ(addr.toPort(), 9090);
    EXPECT_EQ(addr.ip(), "192.168.1.1");

    InetAddress ipv6Addr("2001:db8::1", 1234, true);
    EXPECT_EQ(ipv6Addr.toIpPort(), "2001:db8::1:1234");
}

// 测试地址结构转换
TEST(InetAddressTest, SockAddrConversion)
{
    sockaddr_in rawAddr;
    memset(&rawAddr, 0, sizeof(rawAddr));
    rawAddr.sin_family = AF_INET;
    rawAddr.sin_port = htons(5678);
    inet_pton(AF_INET, "10.0.0.1", &rawAddr.sin_addr);

    InetAddress addr;
    addr.setSockAddr(reinterpret_cast<sockaddr *>(&rawAddr), sizeof(rawAddr));
    EXPECT_EQ(addr.toIpPort(), "10.0.0.1:5678");
}