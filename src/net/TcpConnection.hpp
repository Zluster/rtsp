#pragma once
#include <string>
#include <memory>
#include <atomic>
#include "Noncopyable.hpp"
#include "Buffer.hpp"
#include "Callbacks.hpp"
#include "InetAddress.hpp"
#include "Timer.hpp"
#include "EventLoop.hpp"
#include "Channel.hpp"
namespace net
{
    class EventLoop;
    class Socket;
    class Channel;
    class TcpConnection : public std::enable_shared_from_this<TcpConnection>, base::Noncopyable
    {
    public:
        TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);

        ~TcpConnection();

        EventLoop *getLoop() const { return loop_; }

        const std::string &getName() const { return name_; }

        const InetAddress &getLocalAddr() const { return localAddr_; }

        const InetAddress &getPeerAddr() const { return peerAddr_; }

        bool connected() const { return state_ == kConnected; }

        void send(const std::string &message);
        void send(Buffer *buf);
        void send(const void *data, size_t len);

        void shutdown();
        void setTcpNoDelay(bool on);

        void setConnectionCallback(const ConnectionCallback cb) { connectionCallback_ = std::move(cb); }

        void setMessageCallback(const MessageCallback cb) { messageCallback_ = std::move(cb); }

        void setWriteCompleteCallback(const WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }
        void setHighWaterMarkCallback(const HighWaterMarkCallback cb, size_t HighWaterMark)
        {
            highWaterMarkCallback_ = std::move(cb);
            HighWaterMark_ = HighWaterMark;
        }
        void setCloseCallback(const CloseCallback cb) { closeCallback_ = std::move(cb); }
        void connectEstablished();
        void connectDestroyed();
        void forceClose();

        static void defaultConnectionCallback(const TcpConnectionPtr &conn);
        static void defaultCloseCallback(const TcpConnectionPtr &conn);
        static void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime);

    private:
        enum StateE
        {
            kDisconnected,
            kConnecting,
            kConnected,
            kDisconnecting
        };
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        void sendInLoop(const std::string &message);
        void sendInLoop(Buffer *buf);
        void sendInLoop(const void *data, size_t len);

        void setState(StateE state) { state_ = state; }
        void shutdownInLoop();
        void forceCloseInLoop();

        EventLoop *loop_;
        const std::string name_;
        std::atomic<StateE> state_;
        int sockfd_;
        std::unique_ptr<Channel> channel_;

        InetAddress localAddr_;
        InetAddress peerAddr_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        CloseCallback closeCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        size_t HighWaterMark_;

        Buffer inputBuffer_;
        Buffer outputBuffer_;
        std::mutex mutex_;
    };
}