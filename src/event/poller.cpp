#include "../headers.h"

#include "../core/core.h"
#include "../log/logger.h"

#include "poller.h"

extern Server *serverPtr;

Poller::Poller()
{
}

Poller::~Poller()
{
}

// ctx is Connection*
bool Poller::addFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    PollCtx pctx(now, ctx);

    if (!fdCtxMap_.count(fd))
    {
        fdCtxMap_.insert({fd, pctx});
    }
    else
    {
        LOG_WARN << "fd:" << fd << " exists";
    }

    return 1;
}

bool Poller::modFd(int fd, EVENTS events, void *ctx)
{
    pollfd now = {fd, events2poll(events), 0};
    PollCtx pctx(now, ctx);

    if (fdCtxMap_.count(fd))
    {
        fdCtxMap_[fd] = pctx;
    }
    else
    {
        LOG_WARN << "fd:" << fd << " doesn't exist";
    }

    return 1;
}

bool Poller::delFd(int fd)
{
    fdCtxMap_.erase(fd);
    return 1;
}

int Poller::processEvents(int flags, int timeoutMs)
{
    pollfd fds[fdCtxMap_.size()];

    int size = 0;
    for (auto &x : fdCtxMap_)
    {
        fds[size++] = x.second.pfd_;
    }

    int ret = poll(fds, size, timeoutMs);
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

        Connection *c = (Connection *)fdCtxMap_[x.fd].ctx_;

        short int revents = x.revents;
        if (revents & (POLLERR | POLLHUP))
        {
            revents |= POLLIN | POLLOUT;
        }
        if (c->quit_ == 1)
        {
            goto recover;
        }

        if ((revents & POLLIN) && c->read_.handler_)
        {
            if (flags & POSTED)
            {
                if (c->read_.type_ == EventType::ACCEPT)
                {
                    postedAcceptEvents_.push_back(&c->read_);
                }
                else
                {
                    postedEvents_.push_back(&c->read_);
                }
            }
            else
            {
                c->read_.handler_(&c->read_);
            }
        }

        if (c->quit_ == 1)
        {
            goto recover;
        }

        if ((revents & POLLOUT) && c->write_.handler_)
        {
            if (flags & POSTED)
            {
                postedEvents_.push_back(&c->write_);
            }
            else
            {
                c->write_.handler_(&c->write_);
            }
        }

    recover:
        if (c->quit_)
        {
            int fd = c->fd_.getFd();
            delFd(fd);
            serverPtr->pool_->recoverConnection(c);
            LOG_INFO << "Connection recover, FD:" << fd;
        }
    }
    return 0;
}

void Poller::processPostedAcceptEvents()
{
    while (!postedAcceptEvents_.empty())
    {
        auto &now = postedAcceptEvents_.front();
        if (now->handler_)
        {
            now->handler_(now);
        }
        postedAcceptEvents_.pop_front();
    }
}

void Poller::processPostedEvents()
{
    while (!postedEvents_.empty())
    {
        auto &now = postedEvents_.front();
        if (now->handler_)
        {
            now->handler_(now);
        }
        postedEvents_.pop_front();
    }
}