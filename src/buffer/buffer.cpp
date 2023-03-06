#include "buffer.h"
#include <assert.h>
#include <cstring> //perror
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h> //readv
#include <unistd.h>  // write

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
char *Buffer::peek()
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
ssize_t Buffer::recvFd(int fd, int *saveErrno, int flags, int n)
{
    char buff[65535];
    const ssize_t len = recv(fd, buff, n, flags);

    if (len < 0)
    {
        *saveErrno = errno;
    }
    else if (len > 0)
    {
        append(buff, len);
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
ssize_t Buffer::sendFd(int fd, int *saveErrno, int flags)
{
    size_t read_size = readableBytes();
    ssize_t len = send(fd, peek(), read_size, flags);
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
    return buffer_.data();
    // return &*buffer_.begin();
}
const char *Buffer::beginPtr() const
{
    return buffer_.data();
    // return &*buffer_.begin();
}
void Buffer::makeSpace(size_t len)
{
    // if (writableBytes() + prependableBytes() < len) // space is not enough
    // {
    //     buffer_.resize(write_pos_ + len + 1, 0);
    // }
    // else // move readable to prependable
    // {
    //     size_t readable = readableBytes();
    //     std::copy(beginPtr() + read_pos_, beginPtr() + write_pos_, beginPtr());
    //     read_pos_ = 0;
    //     write_pos_ = read_pos_ + readable;
    //     assert(readable == readableBytes());
    // }
    buffer_.resize(write_pos_ + len + 1);
}

LinkedBufferNode::LinkedBufferNode()
{
    start = (u_char *)malloc(NODE_SIZE);
    end = start + NODE_SIZE;
    pos = 0;
    len = 0;
    prev = NULL;
    next = NULL;
}

void LinkedBufferNode::init()
{
    free(start);
    start = (u_char *)malloc(NODE_SIZE);
    end = start + NODE_SIZE;
    pos = 0;
    len = 0;
    prev = NULL;
    next = NULL;
}

std::string LinkedBufferNode::toString()
{
    return std::string(start + pos, start + pos + len);
}

LinkedBufferNode::~LinkedBufferNode()
{
    free(start);
    len = 0;
}

LinkedBuffer::LinkedBuffer()
{
    nodes.emplace_back();
    now = &nodes.front();
    allread = 0;
}

void LinkedBuffer::init()
{
    nodes.clear();
    nodes.emplace_back();
    now = &nodes.front();
    allread = 0;
}

ssize_t LinkedBuffer::recvFd(int fd, int *saveErrno, int flags)
{
    allread = 0;
    auto &nowr = nodes.back();

    int n = recv(fd, nowr.start + nowr.len, NODE_SIZE - nowr.len, flags);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n > 0)
    {
        nowr.len += n;
        if (nowr.len == NODE_SIZE)
        {
            auto newNode = LinkedBufferNode();
            nowr.next = &newNode;
            newNode.prev = &nowr;
            nodes.push_back(newNode);
        }
    }
    return n;
}

ssize_t LinkedBuffer::sendFd(int fd, int *saveErrno, int flags)
{
    int n = send(fd, now->start + now->pos, now->len - now->pos, flags);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n > 0)
    {
        now->pos += n;
        if (now->pos == now->len)
        {
            if (now->next == NULL)
            {
                allread = 1;
            }
            else
            {
                now = now->next;
            }
        }
    }
    return n;
}

void LinkedBuffer::append(u_char *data, size_t len)
{
    allread = 0;
    size_t copied = 0;
    auto now = &nodes.back();

    while (len - copied > 0)
    {
        if (len - copied > NODE_SIZE - now->len)
        {
            std::copy(data + copied, data + copied + NODE_SIZE - now->len, now->start + now->len);
            copied += NODE_SIZE - now->len;
            now->len += NODE_SIZE - now->len;
            if (now->len == NODE_SIZE)
            {
                auto newNode = LinkedBufferNode();
                now->next = &newNode;
                newNode.prev = now;
                nodes.push_back(newNode);

                now = now->next;
            }
        }
        else
        {
            std::copy(data + copied, data + len, now->start + now->len);
            now->len += len;
            return;
        }
    }
    return;
}

void LinkedBuffer::append(const char *data, size_t len)
{
    allread = 0;
    size_t copied = 0;
    auto now = &nodes.back();

    while (len - copied > 0)
    {
        if (len - copied > NODE_SIZE - now->len)
        {
            std::copy(data + copied, data + copied + NODE_SIZE - now->len, now->start + now->len);
            copied += NODE_SIZE - now->len;
            now->len += NODE_SIZE - now->len;
            if (now->len == NODE_SIZE)
            {
                auto newNode = LinkedBufferNode();
                now->next = &newNode;
                newNode.prev = now;
                nodes.push_back(newNode);

                now = now->next;
            }
        }
        else
        {
            std::copy(data + copied, data + len, now->start + now->len);
            now->len += len;
            return;
        }
    }
    return;
}

void LinkedBuffer::append(const std::string &str)
{
    allread = 0;
    append(str.c_str(), str.length());
    return;
}

void LinkedBuffer::retrieve(size_t len)
{
    while (len > 0 && allread != 1)
    {
        if (len <= now->len - now->pos) // last node
        {
            now->pos += len;
            len = 0;
        }
        else
        {
            now->pos = now->len;
            len -= now->len - now->pos;
            if (now->next == NULL)
            {
                allread = 1;
            }
            else
            {
                now = now->next;
            }
        }
    }
    return;
}