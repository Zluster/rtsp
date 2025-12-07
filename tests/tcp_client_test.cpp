#include <gtest/gtest.h>
#include "net/TcpClient.hpp"
#include "net/EventLoop.hpp"
#include "net/InetAddress.hpp"
#include "net/TcpServer.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace net;

// 测试客户端连接服务器
TEST(TcpClientTest, ConnectToServer)
{
    // 启动测试服务器
    EventLoop serverLoop;
    InetAddress serverAddr(9878);
    TcpServer server(&serverLoop, serverAddr, "TestServerForClient");
    std::atomic<bool> serverConnected{false};
    server.setConnectionCallback([&](const TcpConnectionPtr &conn)
                                 {
        if (conn->connected()) {
            serverConnected = true;
        } });
    server.start();

    // 启动客户端
    EventLoop clientLoop;
    TcpClient client(&clientLoop, serverAddr, "TestClient");
    std::atomic<bool> clientConnected{false};
    client.setConnectionCallback([&](const TcpConnectionPtr &conn)
                                 {
        if (conn->connected()) {
            clientConnected = true;
            client.disconnect();  // 连接后立即断开
            clientLoop.quit();
        } });

    std::thread serverThread([&]()
                             { serverLoop.runAfter(1.0, [&]() { serverLoop.quit(); }); serverLoop.loop(); });
    std::thread clientThread([&]()
                             { client.connect(); clientLoop.loop(); });

    serverThread.join();
    clientThread.join();

    EXPECT_TRUE(serverConnected);
    EXPECT_TRUE(clientConnected);
}

// 测试客户端消息发送
TEST(TcpClientTest, SendMessage)
{
    EventLoop serverLoop;
    InetAddress serverAddr(9879);
    TcpServer server(&serverLoop, serverAddr, "MsgServer");
    std::atomic<int> msgReceived{0};
    server.setMessageCallback([&](const TcpConnectionPtr &conn, Buffer *buf, base::Timestamp)
                              {
        if (buf->readableBytes() > 0) {
            msgReceived++;
            conn->shutdown();
        } });
    server.start();

    EventLoop clientLoop;
    TcpClient client(&clientLoop, serverAddr, "MsgClient");
    std::atomic<bool> connected{false};
    client.setConnectionCallback([&](const TcpConnectionPtr &conn)
                                 {
        if (conn->connected()) {
            connected = true;
            conn->send("hello");  // 发送测试消息
        } });

    std::thread serverThread([&]()
                             { serverLoop.runAfter(1.0, [&]() { serverLoop.quit(); }); serverLoop.loop(); });
    std::thread clientThread([&]()
                             { client.connect(); clientLoop.runAfter(0.5, [&]() { clientLoop.quit(); }); clientLoop.loop(); });

    serverThread.join();
    clientThread.join();

    EXPECT_TRUE(connected);
    EXPECT_EQ(msgReceived, 1);
}