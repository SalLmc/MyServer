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

Event::Event(Connection *cc) : c(cc), type(NORMAL), timeout(NOT_TIMEOUT)
{
}

Connection::Connection() : read_(this), write_(this), idx_(-1), server_idx_(-1), data_(NULL), ups_(NULL), quit(0)
{
}

ConnectionPool::ConnectionPool()
{
    cPool_ = (Connection **)malloc(POOLSIZE * sizeof(Connection *));
    for (int i = 0; i < POOLSIZE; i++)
    {
        cPool_[i] = new Connection;
    }
}

ConnectionPool::~ConnectionPool()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        delete cPool_[i];
    }
    free(cPool_);
}

Connection *ConnectionPool::getNewConnection()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        if (cPool_[i]->idx_ == -1)
        {
            cPool_[i]->idx_ = i;
            return cPool_[i];
        }
    }

    auto c = heap.hNew<Connection>();
    c->idx_ = -2;
    return c;
}

void ConnectionPool::recoverConnection(Connection *c)
{
    if (c == NULL)
    {
        return;
    }

    if (c->idx_ == -1)
    {
        return;
    }

    if (c->idx_ == -2)
    {
        heap.hDelete(c);
        // set c to NULL is meaningless. Since c is a uint64
        // c = NULL;
        return;
    }

    c->idx_ = -1;

    if (c->fd_ != -1)
    {
        close(c->fd_.getFd());
        c->fd_ = -1;
    }

    c->read_.handler = std::function<int(Event *)>();
    c->write_.handler = std::function<int(Event *)>();
    c->read_.timeout = c->write_.timeout = NOT_TIMEOUT;

    c->readBuffer_.init();
    c->writeBuffer_.init();

    c->data_.reset();
    c->ups_.reset();
}

ServerAttribute::ServerAttribute(int portt, std::string &&roott, std::string &&indexx, std::string &&from,
                                 std::string &&to, bool auto_indexx, std::vector<std::string> &&tryfiles,
                                 std::vector<std::string> &&auth_path)
    : port(portt), root(roott), index(indexx), from(from), to(to), auto_index(auto_indexx), try_files(tryfiles),
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

sharedMemory::sharedMemory() : addr_(NULL)
{
}

sharedMemory::sharedMemory(size_t size) : size_(size)
{
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    assert(addr_ != MAP_FAILED);
}

int sharedMemory::createShared(size_t size)
{
    size_ = size;
    addr_ = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (addr_ == MAP_FAILED)
    {
        return ERROR;
    }
    return OK;
}

sharedMemory::~sharedMemory()
{
    if (addr_ != NULL)
    {
        munmap(addr_, size_);
    }
}

void *sharedMemory::getAddr()
{
    return addr_;
}

FileInfo::FileInfo(std::string &&namee, unsigned char typee, off_t size_bytee, timespec mtimee)
    : name(namee), type(typee), size_byte(size_bytee), mtime(mtimee)
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

str_t::str_t(u_char *dt, size_t l) : data(dt), len(l)
{
}

std::string str_t::toString()
{
    return std::string(data, data + len);
}