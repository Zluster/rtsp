#pragma once
#include <functional>
#include <memory>
#include "Buffer.hpp"
#include "InetAddress.hpp"
#include "Timer.hpp"
namespace net
{
    class TcpConnection;
    class EventLoop;
    class Session;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using SessionPtr = std::shared_ptr<Session>;

    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, base::Timestamp)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
    using ErrorCallback = std::function<void(const TcpConnectionPtr &, const std::string &)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
    using SessionCallback = std::function<void(const SessionPtr &)>;
    using SessionCloseCallback = std::function<void(const SessionPtr &)>;
}