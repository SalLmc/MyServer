#include "core.h"

Cycle *cyclePtr;
ConnectionPool cPool;

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

Event::Event(Connection *cc) : handler(NULL), c(cc), type(NORMAL), timeout(NOT_TIMEOUT)
{
}

Connection::Connection() : read_(this), write_(this), idx_(-1), server_idx_(-1), data(NULL)
{
}

ConnectionPool::ConnectionPool() : flags(0)
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
        if (!(flags >> i))
        {
            flags |= (1 << i);
            cPool_[i]->idx_ = i;
            return cPool_[i];
        }
    }
    return NULL;
}

// #define RE_ALLOC

void ConnectionPool::recoverConnection(Connection *c)
{
    uint8_t recover = ~(1 << c->idx_);
    flags &= recover;

#ifdef RE_ALLOC
    cPool_[c->idx_] = new Connection;
    delete c;
#else
    c->idx_ = -1;

    if (c->fd_ != -1)
    {
        close(c->fd_.getFd());
        c->fd_ = -1;
    }

    c->read_.handler = c->write_.handler = NULL;
    c->read_.timeout = c->write_.timeout = NOT_TIMEOUT;

    c->readBuffer_.retrieveAll();
    c->writeBuffer_.retrieveAll();

    c->data = NULL;
#endif
}

ServerAttribute::ServerAttribute(int portt, std::string &&roott, std::string &&indexx, std::string &&from,
                                 std::string &&to, int auto_indexx, std::vector<std::string> &&tryfiles)
    : port(portt), root(roott), index(indexx), auto_index(auto_indexx), try_files(tryfiles)
{
    proxy_pass.from = from;
    proxy_pass.to = to;
}

Cycle::Cycle(ConnectionPool *pool, Logger *logger) : pool_(pool), logger_(logger)
{
}

Cycle::~Cycle()
{
    if (logger_ != NULL)
    {
        delete logger_;
        logger_ = NULL;
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
    int ret = name.compare(other.name);
    if (ret != 0)
    {
        return ret;
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