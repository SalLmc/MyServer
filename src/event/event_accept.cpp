#include "../headers.h"

#include "../core/core.h"
#include "../event/epoller.h"
#include "../global.h"
#include "../utils/utils_declaration.h"

sharedMemory shmForAMtx;
ProcessMutex acceptMutex;
extern Cycle *cyclePtr;

int shmtxCreate(ProcessMutex *mtx, ProcessMutexShare *addr)
{
    mtx->lock = &addr->lock;

    if (mtx->spin == (unsigned long)-1)
    {
        return OK;
    }

    mtx->spin = 2048;
    mtx->wait = &addr->wait;

    if (sem_init(&mtx->sem, 1, 0) == -1)
    {
        LOG_CRIT << "sem_init failed";
        return ERROR;
    }
    else
    {
        mtx->semaphore = 1;
    }

    return OK;
}

int shmtxTryLock(ProcessMutex *mtx)
{
    return (*mtx->lock == 0 && __sync_bool_compare_and_swap(mtx->lock, 0, getpid()));
}

void shmtxLock(ProcessMutex *mtx)
{
    pid_t pid = getpid();
    unsigned long i, n;

    for (;;)
    {
        if (*mtx->lock == 0 && __sync_bool_compare_and_swap(mtx->lock, 0, pid))
        {
            return;
        }

        for (n = 1; n < mtx->spin; n <<= 1)
        {

            for (i = 0; i < n; i++)
            {
                asm_pause();
            }

            if (*mtx->lock == 0 && __sync_bool_compare_and_swap(mtx->lock, 0, pid))
            {
                return;
            }
        }

        if (mtx->semaphore)
        {
            __sync_fetch_and_add(mtx->wait, 1);

            if (*mtx->lock == 0 && __sync_bool_compare_and_swap(mtx->lock, 0, pid))
            {
                __sync_fetch_and_add(mtx->wait, -1);
                return;
            }

            LOG_INFO << "sem wait";

            while (sem_wait(&mtx->sem) == -1)
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
    if (__sync_bool_compare_and_swap(mtx->lock, getpid(), 0))
    {
        volatile unsigned long wait;

        if (!mtx->semaphore)
        {
            return;
        }

        for (;;)
        {
            wait = *mtx->wait;
            if ((volatile long)wait <= 0)
            {
                return;
            }
            if (__sync_bool_compare_and_swap(mtx->wait, wait, wait - 1))
            {
                break;
            }
        }

        if (sem_post(&mtx->sem) == -1)
        {
            LOG_INFO << "sem_post() failed while wake shmtx";
        }
    }
}

int acceptexTryLock(Cycle *cycle)
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
            if (cyclePtr->multiplexer->addFd(listen->fd_.getFd(), EVENTS(IN | ET), listen) == 0)
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
            if (cyclePtr->multiplexer->delFd(listen->fd_.getFd()) == 0)
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
    if (semaphore)
    {
        if (sem_destroy(&sem) == -1)
        {
            LOG_WARN << "sem_destroy failed";
        }
    }
}