#pragma once
#include <functional>
#include <memory>
#include "Timer.hpp"
namespace net
{
    class EventLoop;
    class Channel
    {
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(base::Timestamp)>;

        Channel(EventLoop *loop, int fd);
        ~Channel();

        void handleEvent(base::Timestamp receiveTime);
        void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
        void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
        void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
        void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

        void enableReading()
        {
            events_ |= kReadEvent;
            update();
        }
        void disableReading()
        {
            events_ &= ~kReadEvent;
            update();
        }
        void enableWriting()
        {
            events_ |= kWriteEvent;
            update();
        }
        void disableWriting()
        {
            events_ &= ~kWriteEvent;
            update();
        }
        void disableAll()
        {
            events_ = kNoneEvent;
            update();
        }

        bool isWriting() const { return events_ & kWriteEvent; }
        bool isReading() const { return events_ & kReadEvent; }

        bool isNoneEvent() const { return events_ == kNoneEvent; }

        int fd() const { return fd_; }

        int events() const { return events_; }

        void setEvents(int events) { events_ = events; }

        int index() const { return index_; }
        void setIndex(int idx) { index_ = idx; }

        void setRevents(int revt) { revents_ = revt; }
        int revents() const { return revents_; }

        EventLoop *ownerLoop() const { return loop_; }

        void remove();

    private:
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;
        EventLoop *loop_;
        const int fd_;
        int events_;
        int revents_;
        int index_;
        bool addedToLoop_;
        bool eventHandling_;

        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
        void update();
    };

    std::string eventsToString(int fd, int ev);
}