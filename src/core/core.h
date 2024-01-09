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

enum Events
{
    IN = 0x001,
    PRI = 0x002,
    OUT = 0x004,
    RDNORM = 0x040,
    RDBAND = 0x080,
    WRNORM = 0x100,
    WRBAND = 0x200,
    MSG = 0x400,
    ERR = 0x008,
    HUP = 0x010,
    RDHUP = 0x2000,
    EXCLUSIVE = 1u << 28,
    WAKEUP = 1u << 29,
    ONESHOT = 1u << 30,
    ET = 1u << 31
};

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
    const static int POOLSIZE = 1024;
    ConnectionPool(int size = ConnectionPool::POOLSIZE);
    ~ConnectionPool();
    Connection *getNewConnection();
    void recoverConnection(Connection *c);
    bool isActive(Connection *c);
    int activeCnt;

  private:
    std::list<Connection *> connectionList_;
    std::unordered_map<uint64_t, bool> poolPtrsActiveMap_;

    std::unordered_set<uint64_t> activeMallocPtrs_;
};

struct ServerAttribute
{
    int port;
    std::string root;
    std::string index;

    std::string from;
    std::vector<std::string> to;
    int idx = 0;

    bool autoIndex;
    std::vector<std::string> tryFiles;

    std::vector<std::string> authPaths;
};

class Server
{
  public:
    Server() = delete;
    Server(Logger *logger);
    ~Server();

    void setServers(const std::vector<ServerAttribute> &servers);
    void setTypes(const std::unordered_map<std::string, std::string> &typeMap);
    int initListen(std::function<int(Event *)> handler);
    void initEvent(bool useEpoll);
    int regisListen(Events events);
    void eventLoop();

    ConnectionPool pool_;
    std::vector<Connection *> listening_;
    std::vector<ServerAttribute> servers_;
    Logger *logger_;
    Multiplexer *multiplexer_;
    HeapTimer timer_;

    std::unordered_map<std::string, std::string> extenContentTypeMap_;
};

enum class ProcessStatus
{
    NOT_USED,
    ACTIVE
};

class Process
{
  public:
    pid_t pid_;
    ProcessStatus status_ = ProcessStatus::NOT_USED;
};

// @return NULL on failed
Connection *setupListen(Server *server, int port);

#endif