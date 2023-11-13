#ifndef BUFFER_H
#define BUFFER_H

#include "../headers.h"

class Buffer
{
  public:
    Buffer(int buffSize = 5120);
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
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};

class LinkedBufferNode
{
  public:
    const static int NODE_SIZE = 4096;

    LinkedBufferNode(size_t size = LinkedBufferNode::NODE_SIZE);
    ~LinkedBufferNode();

    void init(size_t size = LinkedBufferNode::NODE_SIZE);
    size_t getSize();
    size_t readableBytes();
    size_t writableBytes();
    std::string toString();
    // @return max(0, len-leftSpace)
    size_t append(const u_char *data, size_t len);
    size_t append(const char *data, size_t len);

    u_char *start;
    u_char *end;

    // read begins at pos
    size_t pos;
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

    bool allRead();
    ssize_t recvFd(int fd, int *saveErrno, int flags);
    ssize_t cRecvFd(int fd, int *saveErrno, int flags);
    ssize_t sendFd(int fd, int *saveErrno, int flags);
    void append(const u_char *data, size_t len);
    void append(const char *data, size_t len);
    void append(const std::string &str);
    void retrieve(size_t len);
    void retrieveAll();
};

#endif // BUFFER_H