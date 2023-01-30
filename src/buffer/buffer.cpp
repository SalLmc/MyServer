#include "buffer.h"

Buffer::Buffer(int buff_size) : buffer_(buff_size), read_pos_(0), write_pos_(0)
{
}

size_t Buffer::readableBytes() const
{
    return write_pos_ - read_pos_;
}
size_t Buffer::writableBytes() const
{
    return buffer_.size() - write_pos_;
}
size_t Buffer::prependableBytes() const
{
    return read_pos_;
}
const char *Buffer::peek() const
{
    return beginPtr() + read_pos_;
}
void Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());
    read_pos_ += len;
}
void Buffer::retrieveUntil(const char *end)
{
    assert(peek() <= end);
    retrieve(end - peek());
}
void Buffer::retrieveAll()
{
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}
std::string Buffer::retrieveAllToStr()
{
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}
std::string Buffer::allToStr()
{
    std::string str(peek(), readableBytes());
    return str;
}
const char *Buffer::beginWriteConst() const
{
    return beginPtr() + write_pos_;
}
char *Buffer::beginWrite()
{
    return beginPtr() + write_pos_;
}
void Buffer::hasWritten(size_t len)
{
    write_pos_ += len;
}
void Buffer::append(const char *str, size_t len)
{
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}
void Buffer::append(const std::string &str)
{
    append(str.data(), str.length());
}
void Buffer::append(const void *data, size_t len)
{
    assert(data);
    append(static_cast<const char *>(data), len);
}
void Buffer::append(const Buffer &buff)
{
    append(buff.peek(), buff.readableBytes());
}
void Buffer::ensureWriteable(size_t len)
{
    if (writableBytes() < len)
    {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char buff[65535];
    iovec iov[2];
    const size_t writable = writableBytes();
    // readv
    iov[0].iov_base = beginPtr() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *saveErrno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        write_pos_ += len;
    }
    else
    {
        write_pos_ = buffer_.size();
        append(buff, len - writable);
    }
    return len;
}
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    size_t read_size = readableBytes();
    ssize_t len = write(fd, peek(), read_size);
    if (len < 0)
    {
        *saveErrno = errno;
        return len;
    }
    read_pos_ += len;
    return len;
}
char *Buffer::beginPtr()
{
    return &*buffer_.begin();
}
const char *Buffer::beginPtr() const
{
    return &*buffer_.begin();
}
void Buffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len) // space is not enough
    {
        buffer_.resize(write_pos_ + len + 1);
    }
    else // move readable to prependable
    {
        size_t readable=readableBytes();
        std::copy(beginPtr()+read_pos_,beginPtr()+write_pos_,beginPtr());
        read_pos_=0;
        write_pos_=read_pos_+readable;
        assert(readable==readableBytes());
    }
}