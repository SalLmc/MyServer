#include "../core/core.h"

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
}