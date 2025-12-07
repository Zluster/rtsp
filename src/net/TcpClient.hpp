#pragma once
#include "TcpConnection.hpp"
#include "Socket.hpp"
#include <memory>
#include <mutex>
#include <cassert>
namespace net
{
    class EventLoop;
    class Connector;
    using ConnectorPtr = std::shared_ptr<Connector>;

    class TcpClient : base::Noncopyable
    {
    public:
        TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &name);
        ~TcpClient();

        void connect();
        void disconnect();
        void stop();

        TcpConnectionPtr connection() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return connection_;
        }

        EventLoop *getLoop() const { return loop_; }
        bool retry() const { return retry_; }
        void enableRetry() { retry_ = true; }

        const std::string &name() const { return name_; }

        void setConnectionCallback(ConnectionCallback cb)
        {
            connectionCallback_ = std::move(cb);
        }

        void setMessageCallback(MessageCallback cb)
        {
            messageCallback_ = std::move(cb);
        }

        void setWriteCompleteCallback(WriteCompleteCallback cb)
        {
            writeCompleteCallback_ = std::move(cb);
        }

    private:
        void newConnection(int sockfd);
        void removeConnection(const TcpConnectionPtr &conn);

        EventLoop *loop_;
        ConnectorPtr connector_;
        const std::string name_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        bool retry_;
        bool connect_;
        mutable std::mutex mutex_;
        TcpConnectionPtr connection_;

        static int nextConnId_;
    };
}