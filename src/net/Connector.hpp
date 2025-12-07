#pragma once
#include "Noncopyable.hpp"
#include "InetAddress.hpp"
#include "Channel.hpp"
#include <functional>
#include <memory>
#include <atomic>

namespace net
{
    class EventLoop;

    class Connector : base::Noncopyable, public std::enable_shared_from_this<Connector>
    {
    public:
        using NewConnectionCallback = std::function<void(int sockfd)>;

        Connector(EventLoop *loop, const InetAddress &serverAddr);
        ~Connector();

        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }

        void start();   // 发起连接
        void restart(); // 重连
        void stop();    // 停止连接

        const InetAddress &serverAddress() const { return serverAddr_; }

    private:
        enum States
        {
            kDisconnected,
            kConnecting,
            kConnected
        };
        static const int kMaxRetryDelayMs = 30 * 1000;
        static const int kInitRetryDelayMs = 500;

        void setState(States s) { state_ = s; }
        void startInLoop();
        void stopInLoop();
        void connect();
        void connecting(int sockfd);
        void handleWrite();
        void handleError();
        void retry(int sockfd);
        int removeAndResetChannel();
        void resetChannel();

        EventLoop *loop_;
        InetAddress serverAddr_;
        std::atomic<bool> connect_; // atomic
        States state_;              // FIXME: use atomic
        std::unique_ptr<Channel> channel_;
        NewConnectionCallback newConnectionCallback_;
        int retryDelayMs_;
    };

    using ConnectorPtr = std::shared_ptr<Connector>;
} // namespace net