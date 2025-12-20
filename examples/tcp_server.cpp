#include <iostream>
#include <string>
#include "../src/net/TcpServer.hpp"
#include "../src/net/EventLoop.hpp"
#include "../src/net/InetAddress.hpp"
#include "../src/base/Logger.hpp"
#include "../src/net/TcpConnection.hpp"
#include "../src/base/Timer.hpp"
using namespace net;

void onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        LOG_INFO("New connection: %s -> %s",
                 conn->getLocalAddr().toIpPort().c_str(),
                 conn->getPeerAddr().toIpPort().c_str());
        conn->send("Welcome to echo server!\r\n");
    }
    else
    {
        LOG_INFO("Connection closed: %s -> %s",
                 conn->getLocalAddr().toIpPort().c_str(),
                 conn->getPeerAddr().toIpPort().c_str());
    }
}

void onMessage(const TcpConnectionPtr &conn, Buffer *buf, base::Timestamp time)
{
    std::string msg = buf->retrieveAllAsString();
    LOG_INFO("Received %zd bytes from %s: %s, at %s",
             msg.size(),
             conn->getPeerAddr().toIpPort().c_str(),
             msg.c_str(), time.toString().c_str());

    // 回显消息
    conn->send(msg);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const std::string log_file = "test_server.log";

    // 手动创建日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    logger->set_log_level(base::LogLevel::DEBUG);
    base::LoggerManager::instance().set_logger(std::move(logger));

    LOG_INFO("Server starting...");

    InetAddress listenAddr(8888);
    EventLoop loop;

    TcpServer server(&loop, listenAddr, "EchoServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(4); // 使用4个IO线程

    server.start();
    LOG_INFO("Server started on %s", listenAddr.toIpPort().c_str());

    loop.loop();

    return 0;
}