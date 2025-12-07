#include <gtest/gtest.h>
#include "net/Socket.hpp"
#include "net/InetAddress.hpp"
#include <fcntl.h>
#include <unistd.h>

using namespace net;

// 测试Socket创建和基本功能
TEST(SocketTest, BasicFunctionality)
{
    Socket sock;
    EXPECT_GE(sock.fd(), 0); // 验证文件描述符有效性

    InetAddress localAddr(12345);
    EXPECT_NO_THROW(sock.bindAddress(localAddr)); // 测试绑定
    EXPECT_NO_THROW(sock.listen());               // 测试监听
}

// 测试Socket选项设置
TEST(SocketTest, Options)
{
    Socket sock;

    // 测试TCP_NODELAY
    sock.setTcpNoDelay(true);
    int optval;
    socklen_t len = sizeof(optval);
    getsockopt(sock.fd(), IPPROTO_TCP, O_NDELAY, &optval, &len);
    EXPECT_EQ(optval, 1);

    // 测试SO_REUSEADDR
    sock.setReuseAddr(true);
    getsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &optval, &len);
    EXPECT_EQ(optval, 1);

    // 测试SO_REUSEPORT
    sock.setReusePort(true);
    getsockopt(sock.fd(), SOL_SOCKET, SO_REUSEPORT, &optval, &len);
    EXPECT_EQ(optval, 1);
}

// 测试地址获取功能
TEST(SocketTest, AddressRetrieval)
{
    Socket sock;
    InetAddress localAddr(54321);
    sock.bindAddress(localAddr);

    // 测试本地地址获取
    InetAddress retrievedLocal = Socket::getLocalAddr(sock.fd());
    EXPECT_EQ(retrievedLocal.toPort(), 54321);
}