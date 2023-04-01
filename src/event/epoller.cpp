#include "../headers.h"

#include "../core/core.h"
#include "../global.h"
#include "epoller.h"

Epoller epoller;
std::list<Event *> posted_accept_events;
std::list<Event *> posted_events;

void process_posted_events(std::list<Event *> *events)
{
    while (!events->empty())
    {
        auto &now = events->front();
        if (now->handler)
        {
            now->handler(now);
        }
        events->pop_front();
    }
}

Epoller::Epoller(int max_event) : epollfd_(-1), events_(max_event)
{
}

Epoller::~Epoller()
{
    if (epollfd_ != -1)
        close(epollfd_);
}

int Epoller::setEpollFd(int fd)
{
    assert(fd >= 0);
    epollfd_ = fd;
    return 0;
}

int Epoller::getFd()
{
    return epollfd_;
}

// ctx is Connection*
bool Epoller::addFd(int fd, uint32_t events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events;

    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

// ctx is Connection*
bool Epoller::modFd(int fd, uint32_t events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events;
    return epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool Epoller::delFd(int fd)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    return epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) == 0;
}

extern Cycle *cyclePtr;

int Epoller::processEvents(int flags, int timeout_ms)
{
    int ret = epoll_wait(epollfd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);
    if (ret == -1)
    {
        return -1;
    }
    for (int i = 0; i < ret; i++)
    {
        Connection *c = (Connection *)events_[i].data.ptr;
        if (c == nullptr || c->idx_ == -1)
        {
            continue;
        }

        if (flags & POST_EVENTS)
        {
            if (c->read_.type == ACCEPT)
            {
                posted_accept_events.push_back(&c->read_);
            }
            else
            {
                posted_events.push_back(&c->read_);
            }
            posted_events.push_back(&c->write_);
            continue;
        }

        if ((events_[i].events & EPOLLIN) && c->read_.handler)
        {
            c->read_.handler(&c->read_);
        }
        if ((events_[i].events & EPOLLOUT) && c->write_.handler)
        {
            c->write_.handler(&c->write_);
        }
    }
    return 0;
}