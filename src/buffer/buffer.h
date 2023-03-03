#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <string>
#include <vector> //readv

class Buffer
{
  public:
    Buffer(int buff_size = 1024);
    ~Buffer() = default;

    size_t writableBytes() const;
    size_t readableBytes() const;
    size_t prependableBytes() const;

    const char *peek() const;
    char *peek();
    void ensureWriteable(size_t len);
    void hasWritten(size_t len);

    void retrieve(size_t len);
    void retrieveUntil(const char *end);

    void retrieveAll();
    std::string retrieveAllToStr();
    std::string allToStr();

    const char *beginWriteConst() const;
    char *beginWrite();

    void append(const std::string &str);
    void append(const char *str, size_t len);
    void append(const void *data, size_t len);
    void append(const Buffer &buff);

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);
    ssize_t recvFd(int fd, int *saveErrno, int flags, int n = 65535);
    ssize_t sendFd(int fd, int *saveErrno, int flags);

  private:
    char *beginPtr();
    const char *beginPtr() const;
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_pos_;
    std::atomic<std::size_t> write_pos_;
};

#endif // BUFFER_H