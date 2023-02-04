#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <vector>

#include "../buffer/buffer.h"

class Connection;

struct Event
{
    std::function<int(Event *)> handler;
    Connection *c;
    int type;
};

class Fd
{
  public:
    Fd();
    Fd(int fd);
    ~Fd();
    int getFd();
    bool operator==(int fd);
    bool operator==(Fd fd);
    bool operator!=(int fd);
    bool operator!=(Fd fd);
    void operator=(int fd);

  private:
    int fd_ = -1;
};

class Connection
{
  public:
    ~Connection()
    {
    }
    Event read_;
    Event write_;
    Fd fd_;
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
    ConnectionPool();
    ~ConnectionPool();
    Connection *getNewConnection();
    void recoverConnection(Connection *c);
};

class Cycle : public std::enable_shared_from_this<Cycle>
{
  public:
    ConnectionPool *pool_;
    std::vector<Connection *> listening_;
    // std::shared_ptr<Cycle> getSharedPtr();
};

#define NOT_USED 0
#define ACTIVE 1

class Process
{
  public:
    Connection channel[2];
    pid_t pid;
    int status = NOT_USED;
};

#endif