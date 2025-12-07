#include "Buffer.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <cstring>
#include <sys/uio.h>
#include <cassert>
namespace net
{
    const char *Buffer::findCRLF() const
    {
        const char *crlf = std::search(peek(), beginWrite(), "\r\n", "\r\n" + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    const char *Buffer::findCRLF(const char *start) const
    {
        const char *crlf = std::search(start, beginWrite(), "\r\n", "\r\n" + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    void Buffer::retrieve(size_t len)
    {
        if (len < writableBytes())
        {
            readIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void Buffer::retrieveUntil(const char *end)
    {
        retrieve(end - peek());
    }
    void Buffer::retrieveAll()
    {
        readIndex_ = writeIndex_ = kCheapPrepend;
    }

    std::string Buffer::retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string Buffer::retrieveAsString(size_t len)
    {
        std::string str(peek(), len);
        retrieve(len);
        return str;
    }

    void Buffer::append(const char *str, size_t len)
    {
        ensureWritableBytes(len);
        std::memcpy(beginWrite(), str, len);
        writeIndex_ += len;
    }

    void Buffer::ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    void Buffer::makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin(), begin() + readIndex_, begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
    }

    ssize_t Buffer::readFd(int fd, int *savedErrno)
    {
        // 创建临时缓冲区以避免缓冲区膨胀
        char extrabuf[65536];
        struct iovec vec[2];
        const size_t writable = writableBytes();
        vec[0].iov_base = begin() + writeIndex_;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof(extrabuf);

        const int iovcnt = writable < sizeof(extrabuf) ? 2 : 1;
        const ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0)
        {
            *savedErrno = errno;
        }
        else if (static_cast<size_t>(n) <= writable)
        {
            writeIndex_ += n;
        }
        else
        {
            writeIndex_ = buffer_.size();
            append(extrabuf, n - writable);
        }

        return n;
    }

    ssize_t Buffer::writeFd(int fd, int *savedErrno)
    {
        const ssize_t n = ::write(fd, peek(), readableBytes());
        if (n < 0)
        {
            *savedErrno = errno;
        }
        else
        {
            readIndex_ += n;
        }
        return n;
    }
}