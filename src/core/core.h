#ifndef CORE_H
#define CORE_H

#include "../headers.h"

#include "../buffer/buffer.h"
#include "../event/epoller.h"
#include "../event/poller.h"
#include "../log/logger.h"
#include "../timer/timer.h"

#define OK 0
#define ERROR -1
#define AGAIN -2
#define DECLINED -3
#define DONE -4

#define LF (u_char)'\n'
#define CR (u_char)'\r'
#define CRLF "\r\n"

class Connection;
class Upstream;
class Request;

enum class TimeoutStatus
{
    NOT_TIMEOUT,
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

class Fd
{
  public:
    Fd();
    Fd(int fd);
    ~Fd();
    int getFd();
    void closeFd();
    bool operator==(int fd);
    bool operator==(Fd fd);
    bool operator!=(int fd);
    bool operator!=(Fd fd);
    void operator=(int fd);
    void reset(Fd &&fd);
    Fd &operator=(const Fd &) = delete;

  private:
    int fd_;
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

  private:
    std::list<Connection *> connectionList_;
    std::vector<Connection *> connectionPtrs_;
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

class SharedMemory
{
  public:
    SharedMemory();
    SharedMemory(size_t size);
    int createShared(size_t size);
    void *getAddr();
    ~SharedMemory();

  private:
    void *addr_;
    size_t size_;
};

class c_str
{
  public:
    c_str() = default;
    c_str(u_char *data, size_t len);
    std::string toString();
    u_char *data_;
    size_t len_;
};

class FileInfo
{
  public:
    FileInfo(std::string &&name, unsigned char type, off_t size_byte, timespec mtime);
    bool operator<(FileInfo &other);
    std::string name_;
    unsigned char type_;
    off_t sizeBytes_;
    timespec mtime_;
};

class Dir
{
  public:
    Dir() = delete;
    Dir(DIR *dir);
    ~Dir();
    int getInfos(std::string root);
    int readDir();
    int getStat();
    DIR *dir_;
    dirent *de_;
    struct stat info_;
    std::vector<FileInfo> infos_;
};

#endif