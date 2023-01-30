#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <fcntl.h>
#include <functional>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../buffer/buffer.h"
#include "../log/nanolog.hpp"

int setnonblocking(int fd);
unsigned long long getTickMs();

class Connection;

struct Event
{
    std::function<int(Event *)> handler;
    Connection *c;
    int type;
};

class Connection
{
  public:
    Event read_;
    Event write_;
    int fd_;
    sockaddr_in addr_;
    Buffer readBuffer_;
    Buffer writeBuffer_;
    int idx_;
};

class ConnectionPool
{
  private:
    Connection **cPool_;
    uint32_t flags;

  public:
    const static int POOLSIZE = 32;
    ConnectionPool()
    {
        flags = 0;
        cPool_ = new Connection *[POOLSIZE];
        for (int i = 0; i < POOLSIZE; i++)
        {
            cPool_[i] = new Connection;
            cPool_[i]->idx_ = -1;
        }
    }
    ~ConnectionPool()
    {
        for (int i = 0; i < POOLSIZE; i++)
        {
            delete cPool_[i];
        }
        delete cPool_;
    }
    Connection *getNewConnection()
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
    void recoverConnection(Connection *c)
    {
        uint8_t recover = ~(1 << c->idx_);
        flags &= recover;
        c->idx_ = -1;
    }
};

#endif