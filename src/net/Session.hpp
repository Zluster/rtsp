#pragma once
#include <memory>
#include "Noncopyable.hpp"
#include "Callbacks.hpp"
#include "TcpConnection.hpp"
namespace net
{
    class Session : public std::enable_shared_from_this<Session>, base::Noncopyable
    {
    public:
        explicit Session(const TcpConnectionPtr &conn);
        ~Session();
        void start();
        void stop();
        void onConnection(const TcpConnectionPtr &conn);
        void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime);
        void onWrite(const TcpConnectionPtr &conn);
        void send(const std::string &message);
        void send(const void *data, size_t len);

    private:
        TcpConnectionPtr conn_;
    }
}