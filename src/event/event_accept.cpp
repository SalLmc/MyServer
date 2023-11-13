#include "../headers.h"

#include "../core/core.h"
#include "../event/epoller.h"
#include "../global.h"
#include "../utils/utils_declaration.h"

SharedMemory shmForAMtx;
ProcessMutex acceptMutex;
extern Server *serverPtr;

int shmtxCreate(ProcessMutex *mtx, ProcessMutexShare *addr)
{
    mtx->lock_ = &addr->lock_;

    if (mtx->spin_ == (unsigned long)-1)
    {
        return OK;
    }

    mtx->spin_ = 2048;
    mtx->wait_ = &addr->wait_;

    if (sem_init(&mtx->sem_, 1, 0) == -1)
    {
        LOG_CRIT << "sem_init failed";
        return ERROR;
    }
    else
    {
        mtx->semaphore_ = 1;
    }

    return OK;
}

int shmtxTryLock(ProcessMutex *mtx)
{
    return (*mtx->lock_ == 0 && __sync_bool_compare_and_swap(mtx->lock_, 0, getpid()));
}

void shmtxLock(ProcessMutex *mtx)
{
    pid_t pid = getpid();
    unsigned long i, n;

    for (;;)
    {
        if (*mtx->lock_ == 0 && __sync_bool_compare_and_swap(mtx->lock_, 0, pid))
        {
            return;
        }

        for (n = 1; n < mtx->spin_; n <<= 1)
        {

            for (i = 0; i < n; i++)
            {
                asm_pause();
            }

            if (*mtx->lock_ == 0 && __sync_bool_compare_and_swap(mtx->lock_, 0, pid))
            {
                return;
            }
        }

        if (mtx->semaphore_)
        {
            __sync_fetch_and_add(mtx->wait_, 1);

            if (*mtx->lock_ == 0 && __sync_bool_compare_and_swap(mtx->lock_, 0, pid))
            {
                __sync_fetch_and_add(mtx->wait_, -1);
                return;
            }

            LOG_INFO << "sem wait";

            while (sem_wait(&mtx->sem_) == -1)
            {
                if (errno != EINTR)
                {
                    LOG_CRIT << "sem_wait() failed while waiting on shmtx";
                    break;
                }
            }

            LOG_INFO << "sem wake";

            continue;
        }

        sched_yield();
    }
}

void shmtxUnlock(ProcessMutex *mtx)
{
    if (__sync_bool_compare_and_swap(mtx->lock_, getpid(), 0))
    {
        volatile unsigned long wait;

        if (!mtx->semaphore_)
        {
            return;
        }

        for (;;)
        {
            wait = *mtx->wait_;
            if ((volatile long)wait <= 0)
            {
                return;
            }
            if (__sync_bool_compare_and_swap(mtx->wait_, wait, wait - 1))
            {
                break;
            }
        }

        if (sem_post(&mtx->sem_) == -1)
        {
            LOG_INFO << "sem_post() failed while wake shmtx";
        }
    }
}

int acceptexTryLock(Server *cycle)
{
    if (shmtxTryLock(&acceptMutex))
    {
        if (acceptMutexHeld)
        {
            // held mutex before
            return 1;
        }

        for (auto &listen : cycle->listening_)
        {
            if (serverPtr->multiplexer_->addFd(listen->fd_.getFd(), EVENTS(IN | ET), listen) == 0)
            {
                LOG_CRIT << "Addfd failed, " << strerror(errno) << " " << acceptMutexHeld;
                shmtxUnlock(&acceptMutex);
                return -1;
            }
        }

        acceptMutexHeld = 1;

        return 1;
    }

    if (acceptMutexHeld)
    {
        for (auto &listen : cycle->listening_)
        {
            if (serverPtr->multiplexer_->delFd(listen->fd_.getFd()) == 0)
            {
                LOG_CRIT << "accept mutex delfd failed, errno:" << errno;
                return -1;
            }
        }

        acceptMutexHeld = 0;
    }

    return 0;
}

ProcessMutex::~ProcessMutex()
{
    if (semaphore_)
    {
        if (sem_destroy(&sem_) == -1)
        {
            LOG_WARN << "sem_destroy failed";
        }
    }
}