#include "../headers.h"

#include "../core/core.h"

#include "poller.h"

extern Cycle *cyclePtr;

Poller poller;

Poller::Poller()
{
}

Poller::~Poller()
{
}

bool Poller::addFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    fdCtxMap_.insert({fd, ctx});
    fds_.push_back(now);
    return 1;
}

bool Poller::modFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    bool has = 0;

    for (size_t i = 0; i < fds_.size(); i++)
    {
        if (fds_[i].fd == fd)
        {
            fds_[i] = now;
            has = 1;
            break;
        }
    }

    if (has)
    {
        fdCtxMap_.erase(fd);
        fdCtxMap_.insert({fd, ctx});
    }

    return 1;
}

bool Poller::delFd(int fd)
{
    for (auto it = fds_.begin(); it != fds_.end(); it++)
    {
        if (it->fd == fd)
        {
            fds_.erase(it);
            break;
        }
    }

    return 1;
}

int Poller::processEvents(int flags, int timeout_ms)
{
    int ret = poll(fds_.data(), fds_.size(), timeout_ms);
    if (ret == -1)
    {
        return -1;
    }
    for (auto &x : fds_)
    {
        if (x.revents == 0)
        {
            continue;
        }

        Connection *c = (Connection *)fdCtxMap_[x.fd];

        short int revents = x.revents;
        if (revents & (POLLERR | POLLHUP))
        {
            revents |= POLLIN | POLLOUT;
        }
        if (c->quit == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLIN) && c->read_.handler)
        {
            c->read_.handler(&c->read_);
        }

        if (c->quit == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLOUT) && c->write_.handler)
        {
            c->write_.handler(&c->write_);
        }

    recover:
        if (c->quit)
        {
            int fd = c->fd_.getFd();
            poller.delFd(fd);
            cyclePtr->pool_->recoverConnection(c);
            LOG_INFO << "Connection recover, FD:" << fd;
        }
    }
    return 0;
}