#include "../headers.h"

#include "basic.h"

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

MemFile::MemFile(int fd) : fd_(fd), addr_(NULL), len_(0)
{
    struct stat st;
    if (fstat(fd_, &st) == -1)
    {
        return;
    }

    len_ = st.st_size;

    void *rc = mmap(NULL, len_, PROT_READ, MAP_PRIVATE, fd_, 0);

    if (rc == MAP_FAILED)
    {
        return;
    }

    addr_ = (const char *)rc;
}

MemFile::~MemFile()
{
    if (addr_ != NULL)
    {
        munmap((void *)addr_, len_);
    }

    if (fd_ >= 0)
    {
        close(fd_);
    }
}