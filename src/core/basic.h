#ifndef BASIC_H
#define BASIC_H

#include "../headers.h"

#define OK 0
#define ERROR -1
#define AGAIN -2
#define DECLINED -3
#define DONE -4

#define LF (u_char)'\n'
#define CR (u_char)'\r'
#define CRLF "\r\n"

class Fd
{
  public:
    Fd();
    Fd(int fd);
    ~Fd();
    int get();
    void close();
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
    void init();
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

class MemFile
{
  public:
    MemFile() = delete;
    MemFile(int fd);
    ~MemFile();
    int fd_;
    const char *addr_;
    size_t len_;
};

#endif