#ifndef CORE_H
#define CORE_H

#include <assert.h>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

#include "../buffer/buffer.h"
#include "../log/logger.h"
#include "../timer/timer.h"

#define OK 0
#define ERROR -1
#define AGAIN -2
#define DECLINED -3

#define LF (u_char)'\n'
#define CR (u_char)'\r'
#define CRLF "\r\n"

class Connection;

#define NOT_TIMEOUT 0
#define TIMEOUT 1
#define IGNORE_TIMEOUT 2

class Event
{
  public:
    Event() = delete;
    Event(Connection *cc);
    std::function<int(Event *)> handler;
    Connection *c;
    int type;
    unsigned timeout : 2;
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
    int fd_;
};

class Connection
{
  public:
    Connection();
    Event read_;
    Event write_;
    Fd fd_;
    sockaddr_in addr_;
    Buffer readBuffer_;
    Buffer writeBuffer_;
    int idx_;
    int server_idx_;
    void *data;
};

class ConnectionPool
{
  private:
    Connection **cPool_;
    uint8_t flags;

  public:
    const static int POOLSIZE = 32;
    ConnectionPool();
    ~ConnectionPool();
    Connection *getNewConnection();
    void recoverConnection(Connection *c);
};

class ServerAttribute
{
  public:
    ServerAttribute(int portt, std::string &&roott, std::string &&indexx, std::string &&from, std::string &&to);
    int port;
    std::string root;
    std::string index;
    struct Proxypass
    {
        std::string from;
        std::string to;
    } proxy_pass;
};

class Cycle
{
  public:
    Cycle() = delete;
    Cycle(ConnectionPool *pool, Logger *logger);
    ~Cycle();
    ConnectionPool *pool_;
    std::vector<Connection *> listening_;
    std::vector<ServerAttribute> servers_;
    Logger *logger_;
    HeapTimer timer_;
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

class sharedMemory
{
  public:
    sharedMemory();
    sharedMemory(size_t size);
    int createShared(size_t size);
    void *getAddr();
    ~sharedMemory();

  private:
    void *addr_;
    size_t size_;
};

struct str_t
{
    size_t len;
    u_char *data;
};

#endif