#include "../headers.h"

#include "core.h"

#include "../memory/memory_manage.hpp"
#include "../utils/utils_declaration.h"

Server *serverPtr;
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

Event::Event(Connection *c) : c_(c), type_(EventType::NORMAL), timeout_(TimeoutStatus::NOT_TIMEOUT)
{
}

void Event::init(Connection *conn)
{
    c_ = conn;
    type_ = EventType::NORMAL;
    timeout_ = TimeoutStatus::NOT_TIMEOUT;
    handler_ = std::function<int(Event *)>();
}

Connection::Connection(ResourceType type)
    : read_(this), write_(this), serverIdx_(-1), request_(NULL), upstream_(NULL), quit_(0), type_(type)
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

    quit_ = 0;

    type_ = type;
}

ConnectionPool::ConnectionPool(int size)
{
    for (int i = 0; i < size; i++)
    {
        Connection *c = new Connection(ResourceType::POOL);
        connectionList_.push_back(c);
        connectionPtrs_.push_back(c);
    }
}

ConnectionPool::~ConnectionPool()
{
    for (auto c : connectionPtrs_)
    {
        delete c;
    }
}

Connection *ConnectionPool::getNewConnection()
{
    if (!connectionList_.empty())
    {
        auto c = connectionList_.front();
        connectionList_.pop_front();
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
        connectionList_.push_back(c);
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
    : port_(port), root_(root), index_(index), from_(from), to_(to), auto_index_(auto_index), tryFiles_(tryfiles),
      authPaths_(auth_path)
{
}

Server::Server(ConnectionPool *pool, Logger *logger) : pool_(pool), logger_(logger), multiplexer_(NULL)
{
}

Server::~Server()
{
    if (logger_ != NULL)
    {
        delete logger_;
        logger_ = NULL;
    }
    if (multiplexer_ != NULL)
    {
        delete multiplexer_;
    }
}

void Server::eventLoop()
{
    int flags = NORMAL;

    unsigned long long nextTick = timer_.getNextTick();
    nextTick = ((nextTick == (unsigned long long)-1) ? -1 : (nextTick - getTickMs()));

    int ret = multiplexer_->processEvents(flags, nextTick);
    if (ret == -1)
    {
        LOG_WARN << "process events errno: " << strerror(errno);
    }

    multiplexer_->processPostedAcceptEvents();

    timer_.tick();

    multiplexer_->processPostedEvents();
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
    : name_(name), type_(type), sizeBytes_(size_byte), mtime_(mtime)
{
}

bool FileInfo::operator<(FileInfo &other)
{
    if (type_ != other.type_)
    {
        return type_ < other.type_;
    }
    int ret = name_.compare(other.name_);
    if (ret != 0)
    {
        return ret < 0;
    }
    if (sizeBytes_ != other.sizeBytes_)
    {
        return sizeBytes_ < other.sizeBytes_;
    }
    return mtime_.tv_sec < other.mtime_.tv_sec;
}

Dir::Dir(DIR *dir) : dir_(dir)
{
}

Dir::~Dir()
{
    closedir(dir_);
}

int Dir::readDir()
{
    de_ = readdir(dir_);
    if (de_)
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
    stat(de_->d_name, &info_);
    return OK;
}

int Dir::getInfos(std::string root)
{
    while (this->readDir() == OK)
    {
        if (strcmp(de_->d_name, ".") == 0 || strcmp(de_->d_name, "..") == 0)
        {
            continue;
        }

        stat((root + "/" + std::string(de_->d_name)).c_str(), &info_);

        if (de_->d_type == DT_DIR)
        {
            infos_.emplace_back(std::string(de_->d_name) + "/", de_->d_type, info_.st_size, info_.st_mtim);
        }
        else
        {
            infos_.emplace_back(de_->d_name, de_->d_type, info_.st_size, info_.st_mtim);
        }
    }
    std::sort(infos_.begin(), infos_.end());
    return OK;
}

c_str::c_str(u_char *data, size_t len) : data_(data), len_(len)
{
}

std::string c_str::toString()
{
    return std::string(data_, data_ + len_);
}