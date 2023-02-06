#include "src/core/core.h"
#include "src/event/event.h"
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

sharedMemory shm;

int main()
{
    assert(shm.createShared(sizeof(ProcessMutexShare)) == 0);
    ProcessMutex mtx;
    ProcessMutexShare *share = (ProcessMutexShare *)shm.getAddr();

    shmtxCreate(&mtx, share);
    shmtxTryLock(&mtx);
    shmtxUnlock(&mtx);
    switch (fork())
    {
    case 0:
        if (shmtxTryLock(&mtx))
        {
            printf("child get lock\n");
        }
    default:;
    }
}