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

class Event
{
  public:
    Event() = delete;
    Event(Connection *c);
    std::function<int(Event *)> handler;
    Connection *c;
    EventType type;
    TimeoutStatus timeout;
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
    Connection();
    Event read_;
    Event write_;
    Fd fd_;
    sockaddr_in addr_;
    LinkedBuffer readBuffer_;
    LinkedBuffer writeBuffer_;
    int idx_;
    int serverIdx_;
    std::shared_ptr<Request> request_;
    std::shared_ptr<Upstream> upstream_;

    int quit;
};

class ConnectionPool
{
  private:
    Connection **cPool_;

  public:
    const static int POOLSIZE = 0;
    ConnectionPool();
    ~ConnectionPool();
    Connection *getNewConnection();
    void recoverConnection(Connection *c);
};

class ServerAttribute
{
  public:
    ServerAttribute(int port, std::string &&root, std::string &&index, std::string &&from, std::string &&to,
                    bool auto_index, std::vector<std::string> &&tryfiles, std::vector<std::string> &&auth_path);
    ServerAttribute() = default;

    int port;
    std::string root;
    std::string index;

    std::string from;
    std::string to;

    bool auto_index;
    std::vector<std::string> try_files;

    std::vector<std::string> auth_path;
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
    Multiplexer *multiplexer;
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
    Connection channel[2];
    pid_t pid;
    ProcessStatus status = ProcessStatus::NOT_USED;
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

class str_t
{
  public:
    str_t() = default;
    str_t(u_char *dt, size_t l);
    std::string toString();
    u_char *data;
    size_t len;
};

class FileInfo
{
  public:
    FileInfo(std::string &&name, unsigned char type, off_t size_byte, timespec mtime);
    bool operator<(FileInfo &other);
    std::string name;
    unsigned char type;
    off_t size_byte;
    timespec mtime;
};

class Dir
{
  public:
    Dir() = delete;
    Dir(DIR *dirr);
    ~Dir();
    int getInfos(std::string root);
    int readDir();
    int getStat();
    DIR *dir;
    dirent *de;
    struct stat info;
    std::vector<FileInfo> infos;
};

#endif