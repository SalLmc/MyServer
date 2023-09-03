#ifndef EVENT_H
#define EVENT_H

#include "../headers.h"

class Cycle;

struct ProcessMutexShare
{
    volatile unsigned long lock;
    volatile unsigned long wait;
};

class ProcessMutex
{
  public:
    ~ProcessMutex();
    volatile unsigned long *lock;
    volatile unsigned long *wait;
    unsigned long semaphore;
    sem_t sem;
    unsigned long spin;
};

int shmtxCreate(ProcessMutex *mtx, ProcessMutexShare *addr);
// 1 for success, 0 for failure
int shmtxTryLock(ProcessMutex *mtx);
void shmtxLock(ProcessMutex *mtx);
void shmtxUnlock(ProcessMutex *mtx);
// 1 for success, 0 for failure, -1 for error
int acceptexTryLock(Cycle *cycle);
// @param ev Event *
int setEventTimeout(TimerArgs arg);

#endif