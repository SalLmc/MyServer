#include "core.h"

Cycle *cyclePtr;
ConnectionPool cPool;

Fd::Fd() : fd_(-1)
{
}

Fd::Fd(int fd) : fd_(fd)
{
}

Fd::~Fd()
{
    if (fd_ != -1)
    {
        close(fd_);
    }
}

int Fd::getFd()
{
    return fd_;
}

bool Fd::operator==(int fd)
{
    return fd == fd_;
}
bool Fd::operator==(Fd fd)
{
    return fd.fd_ == fd_;
}
bool Fd::operator!=(int fd)
{
    return fd != fd_;
}
bool Fd::operator!=(Fd fd)
{
    return fd.fd_ != fd_;
}
void Fd::operator=(int fd)
{
    fd_ = fd;
}

Event::Event(Connection *cc) : handler(NULL), c(cc), timeout(NOT_TIMEOUT)
{
}

Connection::Connection() : read_(this), write_(this)
{
}

ConnectionPool::ConnectionPool()
{
    flags = 0;
    cPool_ = new Connection *[POOLSIZE];
    for (int i = 0; i < POOLSIZE; i++)
    {
        cPool_[i] = new Connection;
        cPool_[i]->idx_ = -1;
    }
}

ConnectionPool::~ConnectionPool()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        delete cPool_[i];
    }
    delete cPool_;
}

Connection *ConnectionPool::getNewConnection()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        if (!(flags >> i))
        {
            flags |= (1 << i);
            cPool_[i]->idx_ = i;
            return cPool_[i];
        }
    }
    return NULL;
}

void ConnectionPool::recoverConnection(Connection *c)
{
    uint8_t recover = ~(1 << c->idx_);
    flags &= recover;
    c->idx_ = -1;
    if (c->fd_ != -1)
    {
        close(c->fd_.getFd());
        c->fd_ = -1;
    }
    c->read_.handler = c->write_.handler = NULL;
    c->read_.timeout = c->write_.timeout = NOT_TIMEOUT;

    c->readBuffer_.retrieveAll();
    c->writeBuffer_.retrieveAll();
}

Cycle::Cycle(ConnectionPool *pool, Logger *logger) : pool_(pool), logger_(logger)
{
}

Cycle::~Cycle()
{
    if (logger_ != NULL)
    {
        delete logger_;
        logger_ = NULL;
    }
}

sharedMemory::sharedMemory() : addr_(NULL)
{
}

sharedMemory::sharedMemory(size_t size) : size_(size)
{
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    assert(addr_ != MAP_FAILED);
}

int sharedMemory::createShared(size_t size)
{
    size_ = size;
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (addr_ == MAP_FAILED)
    {
        return ERROR;
    }
    return OK;
}

sharedMemory::~sharedMemory()
{
    if (addr_ != NULL)
    {
        munmap(addr_, size_);
    }
}

void *sharedMemory::getAddr()
{
    return addr_;
}
