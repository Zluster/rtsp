#pragma once
#include <map>
#include <string>
#include <memory>
#include <atomic>
#include "Callbacks.hpp"
#include "InetAddress.hpp"
#include "Noncopyable.hpp"

namespace net
{
    class EventLoop;
    class EventLoopThreadPool;
    class Acceptor;

    class TcpServer : base::Noncopyable
    {
    public:
        TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, bool reuseport = false);
        ~TcpServer();

        const std::string &ipPort() const { return ipPort_; }
        const std::string &name() const { return name_; }

        EventLoop *getLoop() const { return loop_; }

        void setThreadNum(int numThreads);

        void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
        void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
        void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

        void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark)
        {
            highWaterMarkCallback_ = std::move(cb);
            highWaterMark_ = highWaterMark;
        }

        void start();

    private:
        void newConnection(int sockfd, const InetAddress &peerAddr);
        void removeConnection(const TcpConnectionPtr &conn);
        void removeConnectionInLoop(const TcpConnectionPtr &conn);

        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

        EventLoop *loop_; // the base loop
        const std::string ipPort_;
        const std::string name_;

        std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
        std::unique_ptr<EventLoopThreadPool> threadPool_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        size_t highWaterMark_;

        std::atomic<bool> started_;
        int nextConnId_;
        ConnectionMap connections_;
    };
} // namespace net
