#ifndef BUFFER_H
#define BUFFER_H

#include "../headers.h"

/// - content <=> [ start_+pre_ , start_+len_ )
/// - readable <=> [ start_+pos_ , start_+len_ )
/// - writable <=> [ start_+len_ , end_ )
class LinkedBufferNode
{
  public:
    const static size_t NODE_SIZE = 4096;

    LinkedBufferNode(size_t size = LinkedBufferNode::NODE_SIZE);
    LinkedBufferNode(LinkedBufferNode &&other);
    ~LinkedBufferNode();

    bool operator==(const LinkedBufferNode &other);
    bool operator!=(const LinkedBufferNode &other);

    size_t getSize();
    size_t readableBytes();
    size_t writableBytes();
    // @return max(0, len-leftSpace)
    size_t append(const u_char *data, size_t len);
    size_t append(const char *data, size_t len);

    std::string toString();
    std::string toStringAll();

  public:
    u_char *start_;
    u_char *end_;

    size_t pre_;
    size_t pos_;
    size_t len_;

    LinkedBufferNode *prev_;
    LinkedBufferNode *next_;
};

class LinkedBuffer
{
  public:
    LinkedBuffer();
    void init();

    std::list<LinkedBufferNode> nodes_;
    LinkedBufferNode *pivot_;
    std::unordered_map<uint64_t, LinkedBufferNode *> memoryMap_;

    void appendNewNode();
    std::list<LinkedBufferNode>::iterator insertNewNode(std::list<LinkedBufferNode>::iterator iter);
    LinkedBufferNode *getNodeByAddr(uint64_t addr);
    bool allRead();
    ssize_t recvOnce(int fd, int flags);
    ssize_t bufferRecv(int fd, int flags);
    ssize_t bufferSend(int fd, int flags);
    void append(const u_char *data, size_t len);
    void append(const char *data, size_t len);
    void append(const std::string &str);
    void retrieve(size_t len);
    void retrieveAll();

    int tryMoveBuffer(void **leftAddr, void **rightAddr);
    bool atSameNode(void *left, void *right);
};

#endif // BUFFER_H