#ifndef EPOLLER_H
#define EPOLLER_H

#include <assert.h> // close()
#include <errno.h>
#include <fcntl.h>     // fcntl()
#include <sys/epoll.h> //epoll_ctl()
#include <unistd.h>    // close()
#include <vector>

class Epoller
{
  public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    int getFd();
    bool addFd(int fd, uint32_t events, void *ctx);
    bool modFd(int fd, uint32_t events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags=0,int timeout_ms = -1);
    int getEventFd(size_t i) const; // const means only allow to read func members
    uint32_t getEvents(size_t i) const;

  private:
    int epollfd_;
    std::vector<epoll_event> events_;
};

#endif