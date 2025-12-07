#pragma once
#include <vector>
#include <string>
#include <algorithm>

namespace net
{
    class Buffer
    {
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;
        explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize), readIndex_(kCheapPrepend), writeIndex_(kCheapPrepend)
        {
        }

        size_t readableBytes() const { return writeIndex_ - readIndex_; }
        size_t writableBytes() const { return buffer_.size() - writeIndex_; }
        size_t prependableBytes() const { return readIndex_; }

        const char *peek() const { return begin() + readIndex_; }

        char *beginWrite() { return begin() + writeIndex_; }
        const char *beginWrite() const { return begin() + writeIndex_; }

        const char *findCRLF() const;
        const char *findCRLF(const char *start) const;

        void retrieve(size_t len);
        void retrieveUntil(const char *delim);
        void retrieveAll();
        std::string retrieveAllAsString();
        std::string retrieveAsString(size_t len);

        void append(const char *data, size_t len);
        void append(const std::string &data)
        {
            append(data.data(), data.size());
        }
        void append(const void *data, size_t len)
        {
            append(static_cast<const char *>(data), len);
        }

        ssize_t readFd(int fd, int *savedErrno);
        ssize_t writeFd(int fd, int *savedErrno);

    private:
        std::vector<char> buffer_;
        size_t readIndex_;
        size_t writeIndex_;

    private:
        const char *begin() const { return &*buffer_.begin(); }
        char *begin() { return &*buffer_.begin(); }
        void ensureWritableBytes(size_t len);
        void makeSpace(size_t len);
    };
}