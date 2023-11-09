#ifndef EVENT_H
#define EVENT_H

#include "../headers.h"

class Cycle;
class Event;

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

enum EVENTS
{
    IN = 0x001,
#define IN IN
    PRI = 0x002,
#define PRI PRI
    OUT = 0x004,
#define OUT OUT
    RDNORM = 0x040,
#define RDNORM RDNORM
    RDBAND = 0x080,
#define RDBAND RDBAND
    WRNORM = 0x100,
#define WRNORM WRNORM
    WRBAND = 0x200,
#define WRBAND WRBAND
    MSG = 0x400,
#define MSG MSG
    ERR = 0x008,
#define ERR ERR
    HUP = 0x010,
#define HUP HUP
    RDHUP = 0x2000,
#define RDHUP RDHUP
    EXCLUSIVE = 1u << 28,
#define EXCLUSIVE EXCLUSIVE
    WAKEUP = 1u << 29,
#define WAKEUP WAKEUP
    ONESHOT = 1u << 30,
#define ONESHOT ONESHOT
    ET = 1u << 31
#define ET ET
};

class Multiplexer
{
  public:
    virtual ~Multiplexer(){};
    virtual bool addFd(int fd, EVENTS events, void *ctx) = 0;
    virtual bool modFd(int fd, EVENTS events, void *ctx) = 0;
    virtual bool delFd(int fd) = 0;
    virtual int processEvents(int flags, int timeoutMs) = 0;
};

uint32_t events2epoll(EVENTS events);
short events2poll(EVENTS events);

void processEventsList(std::list<Event *> *events);

int shmtxCreate(ProcessMutex *mtx, ProcessMutexShare *addr);
// 1 for success, 0 for failure
int shmtxTryLock(ProcessMutex *mtx);
void shmtxLock(ProcessMutex *mtx);
void shmtxUnlock(ProcessMutex *mtx);
// 1 for success, 0 for failure, -1 for error
int acceptexTryLock(Cycle *cycle);
// @param ev Event *
int setEventTimeout(void *ev);

#endif