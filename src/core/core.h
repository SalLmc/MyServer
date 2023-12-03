#ifndef CORE_H
#define CORE_H

#include "../headers.h"

#include "basic.h"

#include "../buffer/buffer.h"
#include "../timer/timer.h"

class Multiplexer;
class Logger;
class Connection;
class Upstream;
class Request;

enum class TimeoutStatus
{
    NOT_TIMED_OUT,
    TIMEOUT,
    IGNORE
};

enum class EventType
{
    NORMAL,
    ACCEPT
};

enum class ResourceType
{
    POOL,
    MALLOC
};

class Event
{
  public:
    Event() = delete;
    Event(Connection *c);
    void init(Connection *conn);
    std::function<int(Event *)> handler_;
    Connection *c_;
    EventType type_;
    TimeoutStatus timeout_;
};

class Connection
{
  public:
    Connection(ResourceType type = ResourceType::MALLOC);

    void init(ResourceType type);

    Event read_;
    Event write_;
    Fd fd_;
    sockaddr_in addr_;
    LinkedBuffer readBuffer_;
    LinkedBuffer writeBuffer_;
    int serverIdx_;
    std::shared_ptr<Request> request_;
    std::shared_ptr<Upstream> upstream_;
    bool quit_;

    ResourceType type_;
};

class ConnectionPool
{
  public:
    const static int POOLSIZE = 2048;
    ConnectionPool(int size = ConnectionPool::POOLSIZE);
    ~ConnectionPool();
    Connection *getNewConnection();
    void recoverConnection(Connection *c);
    int activeCnt;

  private:
    std::list<Connection *> connectionList_;
    std::vector<Connection *> connectionPtrs_;

    std::unordered_set<uint64_t> savePtrs_;
};

class ServerAttribute
{
  public:
    ServerAttribute(int port, std::string &&root, std::string &&index, std::string &&from, std::string &&to,
                    bool auto_index, std::vector<std::string> &&tryfiles, std::vector<std::string> &&auth_path);
    ServerAttribute() = default;

    int port_;
    std::string root_;
    std::string index_;

    std::string from_;
    std::string to_;

    bool auto_index_;
    std::vector<std::string> tryFiles_;

    std::vector<std::string> authPaths_;
};

class Server
{
  public:
    Server() = delete;
    Server(ConnectionPool *pool, Logger *logger);
    ~Server();

    void eventLoop();

    ConnectionPool *pool_;
    std::vector<Connection *> listening_;
    std::vector<ServerAttribute> servers_;
    Logger *logger_;
    Multiplexer *multiplexer_;
    HeapTimer timer_;
};

enum class ProcessStatus
{
    NOT_USED,
    ACTIVE
};

class Process
{
  public:
    Connection channel_[2];
    pid_t pid_;
    ProcessStatus status_ = ProcessStatus::NOT_USED;
};

#endif