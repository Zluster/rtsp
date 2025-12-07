#include <gtest/gtest.h>
#include "net/TcpServer.hpp"
#include "net/EventLoop.hpp"
#include "net/InetAddress.hpp"
#include "net/TcpConnection.hpp" // 添加这一行
#include <thread>
#include <chrono>
#include <atomic>

using namespace net;

// 测试服务器启动和连接处理
TEST(TcpServerTest, StartAndAccept)
{
    EventLoop loop;
    InetAddress listenAddr(9876);
    TcpServer server(&loop, listenAddr, "TestServer");

    std::atomic<int> connectionCount{0};
    server.setConnectionCallback([&](const TcpConnectionPtr &conn)
                                 {
        if (conn->connected()) {
            connectionCount++;
            conn->shutdown();  // 连接后立即关闭
        } });

    server.start();
    std::thread clientThread([&]()
                             {
        // 启动客户端连接服务器
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9876);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        close(sockfd); });

    loop.runAfter(0.5, [&]()
                  { loop.quit(); }); // 0.5秒后退出事件循环
    loop.loop();
    clientThread.join();

    EXPECT_EQ(connectionCount, 1); // 验证连接被正确处理
}

// 测试多线程模式
TEST(TcpServerTest, MultiThreadMode)
{
    EventLoop loop;
    InetAddress listenAddr(9877);
    TcpServer server(&loop, listenAddr, "MultiThreadServer");
    server.setThreadNum(2); // 设置2个IO线程

    std::atomic<int> msgCount{0};
    server.setMessageCallback([&](const TcpConnectionPtr &conn, Buffer *buf, base::Timestamp)
                              {
                                  msgCount++;
                                  conn->send(buf); // 回显消息
                              });

    server.start();
    std::thread clientThread([&]()
                             {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9877);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        
        const char* msg = "test";
        send(sockfd, msg, strlen(msg), 0);
        char buf[1024];
        recv(sockfd, buf, sizeof(buf), 0);
        close(sockfd); });

    loop.runAfter(0.5, [&]()
                  { loop.quit(); });
    loop.loop();
    clientThread.join();

    EXPECT_EQ(msgCount, 1); // 验证消息被正确处理
}