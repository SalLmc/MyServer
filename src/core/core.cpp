#include "../headers.h"

#include "core.h"

#include "../memory/memory_manage.hpp"

Cycle *cyclePtr;
ConnectionPool cPool;
extern HeapMemory heap;

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

void Fd::closeFd()
{
    if (fd_ != -1)
    {
        close(fd_);
    }
    fd_ = -1;
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
void Fd::reset(Fd &&fd)
{
    closeFd();
    fd_ = fd.fd_;
    fd.fd_ = -1;
}

Event::Event(Connection *c) : c(c), type(EventType::NORMAL), timeout(TimeoutStatus::NOT_TIMEOUT)
{
}

void Event::init(Connection *conn)
{
    c = conn;
    type = EventType::NORMAL;
    timeout = TimeoutStatus::NOT_TIMEOUT;
    handler = std::function<int(Event *)>();
}

Connection::Connection(ResourceType type)
    : read_(this), write_(this), serverIdx_(-1), request_(NULL), upstream_(NULL), quit(0), type_(type)
{
}

void Connection::init(ResourceType type)
{
    read_.init(this);
    write_.init(this);

    if (fd_ != -1)
    {
        close(fd_.getFd());
        fd_ = -1;
    }

    readBuffer_.init();
    writeBuffer_.init();

    serverIdx_ = -1;

    request_.reset();
    upstream_.reset();

    quit = 0;

    type_ = type;
}

ConnectionPool::ConnectionPool(int size)
{
    for (int i = 0; i < size; i++)
    {
        Connection *c = new Connection(ResourceType::POOL);
        connectionList.push_back(c);
        connectionPtrs.push_back(c);
    }
}

ConnectionPool::~ConnectionPool()
{
    for (auto c : connectionPtrs)
    {
        delete c;
    }
}

Connection *ConnectionPool::getNewConnection()
{
    if (!connectionList.empty())
    {
        auto c = connectionList.front();
        connectionList.pop_front();
        return c;
    }
    else
    {
        return heap.hNew<Connection>(ResourceType::MALLOC);
    }
}

void ConnectionPool::recoverConnection(Connection *c)
{
    if (c == NULL)
    {
        return;
    }

    if (c->type_ == ResourceType::POOL)
    {
        c->init(ResourceType::POOL);
        connectionList.push_back(c);
    }
    else
    {
        heap.hDelete(c);
    }

    return;
}

ServerAttribute::ServerAttribute(int port, std::string &&root, std::string &&index, std::string &&from,
                                 std::string &&to, bool auto_index, std::vector<std::string> &&tryfiles,
                                 std::vector<std::string> &&auth_path)
    : port(port), root(root), index(index), from(from), to(to), auto_index(auto_index), try_files(tryfiles),
      auth_path(auth_path)
{
}

Cycle::Cycle(ConnectionPool *pool, Logger *logger) : pool_(pool), logger_(logger), multiplexer(NULL)
{
}

Cycle::~Cycle()
{
    if (logger_ != NULL)
    {
        delete logger_;
        logger_ = NULL;
    }
    if (multiplexer != NULL)
    {
        delete multiplexer;
    }
}

SharedMemory::SharedMemory() : addr_(NULL)
{
}

SharedMemory::SharedMemory(size_t size) : size_(size)
{
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    assert(addr_ != MAP_FAILED);
}

int SharedMemory::createShared(size_t size)
{
    size_ = size;
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (addr_ == MAP_FAILED)
    {
        return ERROR;
    }
    return OK;
}

SharedMemory::~SharedMemory()
{
    if (addr_ != NULL)
    {
        munmap(addr_, size_);
    }
}

void *SharedMemory::getAddr()
{
    return addr_;
}

FileInfo::FileInfo(std::string &&name, unsigned char type, off_t size_byte, timespec mtime)
    : name(name), type(type), size_byte(size_byte), mtime(mtime)
{
}

bool FileInfo::operator<(FileInfo &other)
{
    if (type != other.type)
    {
        return type < other.type;
    }
    int ret = name.compare(other.name);
    if (ret != 0)
    {
        return ret < 0;
    }
    if (size_byte != other.size_byte)
    {
        return size_byte < other.size_byte;
    }
    return mtime.tv_sec < other.mtime.tv_sec;
}

Dir::Dir(DIR *dirr) : dir(dirr)
{
}

Dir::~Dir()
{
    closedir(dir);
}

int Dir::readDir()
{
    de = readdir(dir);
    if (de)
    {
        return OK;
    }
    else
    {
        return ERROR;
    }
}

int Dir::getStat()
{
    stat(de->d_name, &info);
    return OK;
}

int Dir::getInfos(std::string root)
{
    while (this->readDir() == OK)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        {
            continue;
        }

        stat((root + "/" + std::string(de->d_name)).c_str(), &info);

        if (de->d_type == DT_DIR)
        {
            infos.emplace_back(std::string(de->d_name) + "/", de->d_type, info.st_size, info.st_mtim);
        }
        else
        {
            infos.emplace_back(de->d_name, de->d_type, info.st_size, info.st_mtim);
        }
    }
    std::sort(infos.begin(), infos.end());
    return OK;
}

c_str::c_str(u_char *data, size_t len) : data(data), len(len)
{
}

std::string c_str::toString()
{
    return std::string(data, data + len);
}