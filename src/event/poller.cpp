#include "../headers.h"

#include "../core/core.h"

#include "poller.h"

extern Cycle *cyclePtr;

Poller::Poller()
{
}

Poller::~Poller()
{
}

bool Poller::addFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    PollCtx pctx(now, ctx);
    fdCtxMap_.insert({fd, pctx});
    return 1;
}

bool Poller::modFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    PollCtx pctx(now, ctx);

    fdCtxMap_.erase(fd);
    fdCtxMap_.insert({fd, pctx});

    return 1;
}

bool Poller::delFd(int fd)
{
    fdCtxMap_.erase(fd);
    return 1;
}

int Poller::processEvents(int flags, int timeout_ms)
{
    pollfd fds[fdCtxMap_.size()];

    int size = 0;
    for (auto &x : fdCtxMap_)
    {
        fds[size++] = x.second.pfd;
    }

    int ret = poll(fds, size, timeout_ms);
    if (ret == -1)
    {
        return -1;
    }
    for (auto &x : fds)
    {
        if (x.revents == 0)
        {
            continue;
        }

        Connection *c = (Connection *)fdCtxMap_[x.fd].ctx;

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
            delFd(fd);
            cyclePtr->pool_->recoverConnection(c);
            LOG_INFO << "Connection recover, FD:" << fd;
        }
    }
    return 0;
}