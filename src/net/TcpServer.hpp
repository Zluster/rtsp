/**
 * @file TcpServer.hpp
 * @author zwz (zwz@email)
 * @brief TCP服务器类
 * @version 0.1
 * @date 2025-12-20
 * @copyright Copyright (c) 2025
 *
 */
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
    /**
     * @brief TCP服务器类，用于管理TCP连接
     *
     * 该类封装了TCP服务器的基本功能，包括监听连接、管理连接池等操作。
     * 使用示例：
     * @code
     * EventLoop loop;
     * InetAddress addr(8000);
     * TcpServer server(&loop, addr, "TestServer");
     * server.start();
     * @endcode
     */
    class TcpServer : base::Noncopyable
    {
    public:
        /**
         * @brief 构造TCP服务器实例
         * @param loop EventLoop对象指针
         * @param listenAddr 监听地址
         * @param name 服务器名称
         * @param reuseport 是否重用端口
         */
        TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, bool reuseport = false);
        ~TcpServer();

        const std::string &ipPort() const { return ipPort_; }
        const std::string &name() const { return name_; }

        EventLoop *getLoop() const { return loop_; }
        /**
         * @brief 设置线程数量
         *
         * 设置处理网络IO事件的线程池大小。
         * 线程数为0表示所有IO操作都在主线程中完成，
         * 线程数大于0则创建对应数量的工作线程。
         *
         * @param numThreads 线程数量
         * @note 必须在start()之前调用
         */
        void setThreadNum(int numThreads);

        void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
        void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
        void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

        void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark)
        {
            highWaterMarkCallback_ = std::move(cb);
            highWaterMark_ = highWaterMark;
        }
        /**
         * @brief 启动服务器
         *
         */
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
