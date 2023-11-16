#include "../headers.h"

#include "buffer.h"

Buffer::Buffer(int buff_size) : buffer_(buff_size), readPos_(0), writePos_(0)
{
}

size_t Buffer::readableBytes() const
{
    return writePos_ - readPos_;
}
size_t Buffer::writableBytes() const
{
    return buffer_.size() - writePos_;
}
size_t Buffer::prependableBytes() const
{
    return readPos_;
}
const char *Buffer::peek() const
{
    return beginPtr() + readPos_;
}
char *Buffer::peek()
{
    return beginPtr() + readPos_;
}
void Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());
    readPos_ += len;
}
void Buffer::retrieveUntil(const char *end)
{
    assert(peek() <= end);
    retrieve(end - peek());
}
void Buffer::retrieveAll()
{
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
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
    return beginPtr() + writePos_;
}
char *Buffer::beginWrite()
{
    return beginPtr() + writePos_;
}
void Buffer::hasWritten(size_t len)
{
    writePos_ += len;
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
ssize_t Buffer::readFd(int fd)
{
    char buff[65535];
    iovec iov[2];
    const size_t writable = writableBytes();
    // readv
    iov[0].iov_base = beginPtr() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        writePos_ += len;
    }
    else
    {
        writePos_ = buffer_.size();
        append(buff, len - writable);
    }
    return len;
}
ssize_t Buffer::recvFd(int fd, int flags, int n)
{
    char buff[65535];
    const ssize_t len = recv(fd, buff, n, flags);

    if (len < 0)
    {
    }
    else if (len > 0)
    {
        append(buff, len);
    }

    return len;
}
ssize_t Buffer::writeFd(int fd)
{
    size_t read_size = readableBytes();
    ssize_t len = write(fd, peek(), read_size);
    if (len < 0)
    {
        return len;
    }
    readPos_ += len;
    return len;
}
ssize_t Buffer::sendFd(int fd, int flags)
{
    size_t read_size = readableBytes();
    ssize_t len = send(fd, peek(), read_size, flags);
    if (len < 0)
    {

        return len;
    }
    readPos_ += len;
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
    buffer_.resize(writePos_ + len + 1);
}

LinkedBufferNode::LinkedBufferNode(size_t size)
{
    start_ = (u_char *)aligned_alloc(LinkedBufferNode::NODE_SIZE, size);
    memset(start_, 0, sizeof(size));
    end_ = start_ + size;
    pos_ = 0;
    len_ = 0;
    prev_ = NULL;
    next_ = NULL;
}

LinkedBufferNode::LinkedBufferNode(LinkedBufferNode &&other)
{
    start_ = other.start_;
    end_ = other.end_;
    pos_ = other.pos_;
    len_ = other.len_;
    prev_ = other.prev_;
    next_ = other.next_;

    other.start_ = NULL;
}

LinkedBufferNode::~LinkedBufferNode()
{
    if (start_ != NULL)
    {
        free(start_);
    }
}

void LinkedBufferNode::init(size_t size)
{
    if (start_ != NULL)
    {
        free(start_);
    }
    start_ = (u_char *)calloc(size, 1);
    end_ = start_ + size;
    pos_ = 0;
    len_ = 0;
    prev_ = NULL;
    next_ = NULL;
}

size_t LinkedBufferNode::getSize()
{
    return end_ - start_;
}

size_t LinkedBufferNode::readableBytes()
{
    return len_ - pos_;
}

size_t LinkedBufferNode::writableBytes()
{
    return end_ - start_ - len_;
}

std::string LinkedBufferNode::toString()
{
    return std::string(start_ + pos_, start_ + len_);
}

size_t LinkedBufferNode::append(const u_char *data, size_t len)
{
    size_t free = writableBytes();
    if (free >= len)
    {
        std::copy(data, data + len, this->start_ + this->len_);
        this->len_ += len;
        return 0;
    }
    else
    {
        std::copy(data, data + free, this->start_ + this->len_);
        this->len_ += free;
        return len - free;
    }
}

size_t LinkedBufferNode::append(const char *data, size_t len)
{
    size_t free = writableBytes();
    if (free >= len)
    {
        std::copy(data, data + len, this->start_ + this->len_);
        this->len_ += len;
        return 0;
    }
    else
    {
        std::copy(data, data + free, this->start_ + this->len_);
        this->len_ += free;
        return len - free;
    }
}

LinkedBuffer::LinkedBuffer()
{
    appendNewNode();
    now_ = &nodes_.front();
}

void LinkedBuffer::init()
{
    nodes_.clear();
    appendNewNode();
    now_ = &nodes_.front();
}

void LinkedBuffer::appendNewNode()
{
    if (!nodes_.empty())
    {
        // connect next, prev pointers
        auto lastBack = &nodes_.back();

        nodes_.emplace_back();
        auto now = &nodes_.back();
        uint64_t start = (uint64_t)now->start_;
        memoryMap.insert({start, now});

        lastBack->next_ = now;
        now->prev_ = lastBack;
    }
    else
    {
        nodes_.emplace_back();
        auto now = &nodes_.back();
        uint64_t start = (uint64_t)now->start_;
        memoryMap.insert({start, now});
    }
}

// insert before iter
std::list<LinkedBufferNode>::iterator LinkedBuffer::insertNewNode(std::list<LinkedBufferNode>::iterator iter)
{
    if (iter == nodes_.begin())
    {
        nodes_.push_front(LinkedBufferNode());
        auto &newNode = nodes_.front();
        newNode.next_ = &(*iter);
        iter->prev_ = &newNode;

        memoryMap.insert({(uint64_t)newNode.start_, &newNode});

        return nodes_.begin();
    }
    else if (iter == nodes_.end())
    {
        auto lastNode = std::prev(iter);
        nodes_.push_back(LinkedBufferNode());
        auto &newNode = nodes_.back();
        newNode.prev_ = &(*lastNode);
        lastNode->next_ = &newNode;

        memoryMap.insert({(uint64_t)newNode.start_, &newNode});

        return std::prev(nodes_.end());
    }
    else
    {
        auto newNode = nodes_.insert(iter, LinkedBufferNode());
        auto prevNode = std::prev(newNode);
        newNode->prev_ = &(*prevNode);
        newNode->next_ = &(*iter);
        prevNode->next_ = &(*newNode);
        iter->prev_ = &(*newNode);

        memoryMap.insert({(uint64_t)newNode->start_, &(*newNode)});

        return newNode;
    }
}

LinkedBufferNode *LinkedBuffer::getNodeByAddr(uint64_t addr)
{
    uint64_t startAddr = addr & (~(LinkedBufferNode::NODE_SIZE - 1));
    if (memoryMap.count(startAddr))
    {
        return memoryMap[startAddr];
    }
    return NULL;
}

bool LinkedBuffer::allRead()
{
    auto &back = nodes_.back();
    return back.readableBytes() == 0;
}

// only recv once
ssize_t LinkedBuffer::recvFdOnce(int fd, int flags)
{
    auto &nowr = nodes_.back();

    int n = recv(fd, nowr.start_ + nowr.len_, nowr.writableBytes(), flags);
    if (n <= 0)
    {
    }
    else
    {
        nowr.len_ += n;
        if (nowr.writableBytes() == 0)
        {
            appendNewNode();
        }
    }
    return n;
}

ssize_t LinkedBuffer::recvFd(int fd, int flags)
{
    ssize_t len = 0;
    int n = 0;
    while (1)
    {
        // auto &nowr = nodes.back();
        // n = recv(fd, nowr.start + nowr.len, nowr.writableBytes(), flags);

        n = recvFdOnce(fd, flags);

        if (n <= 0)
        {
            break;
        }
        else
        {
            len += n;
        }
    }

    return len > 0 ? len : n;
}

// only send once. check allread to know whether data left
ssize_t LinkedBuffer::sendFd(int fd, int flags)
{
    int n = send(fd, now_->start_ + now_->pos_, now_->len_ - now_->pos_, flags);
    if (n < 0)
    {
    }
    else if (n > 0)
    {
        now_->pos_ += n;
        if (now_->readableBytes() == 0)
        {
            if (now_->next_ == NULL)
            {
            }
            else
            {
                now_ = now_->next_;
            }
        }
    }
    return n;
}

void LinkedBuffer::append(const u_char *data, size_t len)
{
    auto now = &nodes_.back();
    size_t done = 0;

    while (1)
    {
        int left = now->append(data + done, len - done);
        if (left == 0)
        {
            break;
        }
        done = len - left;

        appendNewNode();
        now = &nodes_.back();
    }
}

void LinkedBuffer::append(const char *data, size_t len)
{
    auto now = &nodes_.back();
    size_t done = 0;

    while (1)
    {
        int left = now->append(data + done, len - done);
        if (left == 0)
        {
            break;
        }
        done = len - left;

        appendNewNode();
        now = &nodes_.back();
    }
}

void LinkedBuffer::append(const std::string &str)
{
    append(str.c_str(), str.length());
    return;
}

void LinkedBuffer::retrieve(size_t len)
{
    while (len > 0 && allRead() != 1)
    {
        if (len <= now_->readableBytes()) // last node
        {
            now_->pos_ += len;
            len = 0;
        }
        else
        {
            len -= now_->readableBytes();
            now_->pos_ = now_->len_;
            if (now_->next_ == NULL)
            {
            }
            else
            {
                now_ = now_->next_;
            }
        }
    }
    return;
}

void LinkedBuffer::retrieveAll()
{
    while (allRead() != 1)
    {
        now_->pos_ = now_->len_;
        if (now_->next_)
        {
            now_ = now_->next_;
        }
    }
    return;
}