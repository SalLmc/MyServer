#include "../headers.h"

#include "buffer.h"

LinkedBufferNode::LinkedBufferNode(size_t size)
{
    start_ = (u_char *)aligned_alloc(LinkedBufferNode::NODE_SIZE, size);
    memset(start_, 0, sizeof(size));
    end_ = start_ + size;
    pre_ = 0;
    pos_ = 0;
    len_ = 0;
    prev_ = NULL;
    next_ = NULL;
}

LinkedBufferNode::LinkedBufferNode(LinkedBufferNode &&other)
{
    start_ = other.start_;
    end_ = other.end_;
    pre_ = other.pre_;
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

bool LinkedBufferNode::operator==(const LinkedBufferNode &other)
{
    return (start_ == other.start_) && (end_ == other.end_);
}

bool LinkedBufferNode::operator!=(const LinkedBufferNode &other)
{
    return (start_ != other.start_) || (end_ != other.end_);
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

std::string LinkedBufferNode::toString()
{
    return std::string(start_ + pos_, start_ + len_);
}

std::string LinkedBufferNode::toStringAll()
{
    return std::string(start_ + pre_, start_ + len_);
}

LinkedBuffer::LinkedBuffer()
{
    appendNewNode();
    pivot_ = &nodes_.front();
}

void LinkedBuffer::init()
{
    memoryMap_.clear();
    nodes_.clear();
    appendNewNode();
    pivot_ = &nodes_.front();
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
        memoryMap_.insert({start, now});

        lastBack->next_ = now;
        now->prev_ = lastBack;
    }
    else
    {
        nodes_.emplace_back();
        auto now = &nodes_.back();
        uint64_t start = (uint64_t)now->start_;
        memoryMap_.insert({start, now});
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

        memoryMap_.insert({(uint64_t)newNode.start_, &newNode});

        return nodes_.begin();
    }
    else if (iter == nodes_.end())
    {
        auto lastNode = std::prev(iter);
        nodes_.push_back(LinkedBufferNode());
        auto &newNode = nodes_.back();
        newNode.prev_ = &(*lastNode);
        lastNode->next_ = &newNode;

        memoryMap_.insert({(uint64_t)newNode.start_, &newNode});

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

        memoryMap_.insert({(uint64_t)newNode->start_, &(*newNode)});

        return newNode;
    }
}

LinkedBufferNode *LinkedBuffer::getNodeByAddr(uint64_t addr)
{
    uint64_t startAddr = addr & (~(LinkedBufferNode::NODE_SIZE - 1));
    if (memoryMap_.count(startAddr))
    {
        return memoryMap_[startAddr];
    }
    return NULL;
}

bool LinkedBuffer::allRead()
{
    auto &back = nodes_.back();
    return back.readableBytes() == 0;
}

// only recv once
ssize_t LinkedBuffer::recvOnce(int fd, int flags)
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

ssize_t LinkedBuffer::bufferRecv(int fd, int flags)
{
    ssize_t len = 0;
    int n = 0;
    while (1)
    {
        // auto &nowr = nodes.back();
        // n = recv(fd, nowr.start + nowr.len, nowr.writableBytes(), flags);

        n = recvOnce(fd, flags);

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
ssize_t LinkedBuffer::bufferSend(int fd, int flags)
{
    int n = send(fd, pivot_->start_ + pivot_->pos_, pivot_->len_ - pivot_->pos_, flags);
    if (n < 0)
    {
    }
    else if (n > 0)
    {
        pivot_->pos_ += n;
        if (pivot_->readableBytes() == 0)
        {
            if (pivot_->next_ == NULL)
            {
            }
            else
            {
                pivot_ = pivot_->next_;
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
        if (len <= pivot_->readableBytes()) // last node
        {
            pivot_->pos_ += len;
            len = 0;
        }
        else
        {
            len -= pivot_->readableBytes();
            pivot_->pos_ = pivot_->len_;
            if (pivot_->next_ == NULL)
            {
            }
            else
            {
                pivot_ = pivot_->next_;
            }
        }
    }
    return;
}

void LinkedBuffer::retrieveAll()
{
    while (allRead() != 1)
    {
        pivot_->pos_ = pivot_->len_;
        if (pivot_->next_)
        {
            pivot_ = pivot_->next_;
        }
    }
    return;
}