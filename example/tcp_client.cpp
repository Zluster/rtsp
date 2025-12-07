#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../src/net/TcpClient.hpp" // 假设已实现TcpClient
#include "../src/net/EventLoop.hpp"
#include "../src/net/InetAddress.hpp"
#include "../src/base/Logger.hpp"
#include "../src/base/Timer.hpp"
using namespace net;

class EchoClient
{
public:
    EchoClient(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop),
          client_(loop, serverAddr, "EchoClient")
    {
        client_.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&EchoClient::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));
    }

    void connect()
    {
        client_.connect();
    }

    void send(const std::string &msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (conn_)
        {
            conn_->send(msg);
        }
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO("Connection %s -> %s %s",
                 conn->getLocalAddr().toIpPort().c_str(),
                 conn->getPeerAddr().toIpPort().c_str(),
                 conn->connected() ? "up" : "down");

        std::lock_guard<std::mutex> lock(mutex_);
        if (conn->connected())
        {
            conn_ = conn;
        }
        else
        {
            conn_.reset();
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, base::Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("Received %zd bytes: %s from %s at %s", msg.size(), msg.c_str(), conn->getPeerAddr().toIpPort().c_str(), time.toString().c_str());
    }

    EventLoop *loop_;
    TcpClient client_;
    std::mutex mutex_;
    TcpConnectionPtr conn_;
};

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const std::string log_file = "test_client.log";

    // 手动创建日志器
    auto logger = std::make_unique<base::SyncLogger>();
    logger->add_sink(std::make_shared<base::FileSink>(log_file));
    logger->add_sink(std::make_shared<base::ConsoleSink>());
    base::LoggerManager::instance().set_logger(std::move(logger));

    LOG_INFO("Client starting...");

    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    EchoClient client(&loop, serverAddr);

    client.connect();

    // 启动一个线程发送消息
    std::thread sender([&client]()
                       {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (int i = 0; i < 5; ++i)
        {
            std::string msg = "Hello, server! This is message " + std::to_string(i) + "\r\n";
            client.send(msg);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } });

    loop.loop();
    sender.join();

    return 0;
}