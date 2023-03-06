#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <list>
#include <string>
#include <vector> //readv

class Buffer
{
  public:
    Buffer(int buff_size = 5120);
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
    ssize_t recvFd(int fd, int *saveErrno, int flags, int n = 65536);
    ssize_t sendFd(int fd, int *saveErrno, int flags);

    char *beginPtr();
    const char *beginPtr() const;

  private:
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_pos_;
    std::atomic<std::size_t> write_pos_;
};

// 5K
#define NODE_SIZE 5120

class LinkedBufferNode
{
  public:
    LinkedBufferNode();
    ~LinkedBufferNode();

    void init();
    std::string toString();

    u_char *start;
    u_char *end;

    size_t pos; // read begins at pos
    size_t len;

    LinkedBufferNode *prev;
    LinkedBufferNode *next;
};

class LinkedBuffer
{
  public:
    LinkedBuffer();
    void init();

    std::list<LinkedBufferNode> nodes;
    LinkedBufferNode *now;
    bool allread;

    ssize_t recvFd(int fd, int *saveErrno, int flags);
    ssize_t sendFd(int fd, int *saveErrno, int flags);
    void append(u_char *data, size_t len);
    void append(const char *data, size_t len);
    void append(const std::string &str);
    void retrieve(size_t len);
};

#endif // BUFFER_H