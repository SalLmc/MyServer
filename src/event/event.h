#ifndef EVENT_H
#define EVENT_H

#include "../headers.h"

class Server;
class Event;

struct ProcessMutexShare
{
    volatile unsigned long lock_;
    volatile unsigned long wait_;
};

class ProcessMutex
{
  public:
    ~ProcessMutex();
    volatile unsigned long *lock_;
    volatile unsigned long *wait_;
    unsigned long semaphore_;
    sem_t sem_;
    unsigned long spin_;
};

// enum EVENTS
// {
//     IN = 0x001,
// #define IN IN
//     PRI = 0x002,
// #define PRI PRI
//     OUT = 0x004,
// #define OUT OUT
//     RDNORM = 0x040,
// #define RDNORM RDNORM
//     RDBAND = 0x080,
// #define RDBAND RDBAND
//     WRNORM = 0x100,
// #define WRNORM WRNORM
//     WRBAND = 0x200,
// #define WRBAND WRBAND
//     MSG = 0x400,
// #define MSG MSG
//     ERR = 0x008,
// #define ERR ERR
//     HUP = 0x010,
// #define HUP HUP
//     RDHUP = 0x2000,
// #define RDHUP RDHUP
//     EXCLUSIVE = 1u << 28,
// #define EXCLUSIVE EXCLUSIVE
//     WAKEUP = 1u << 29,
// #define WAKEUP WAKEUP
//     ONESHOT = 1u << 30,
// #define ONESHOT ONESHOT
//     ET = 1u << 31
// #define ET ET
// };

enum EVENTS
{
    IN = 0x001,
    PRI = 0x002,
    OUT = 0x004,
    RDNORM = 0x040,
    RDBAND = 0x080,
    WRNORM = 0x100,
    WRBAND = 0x200,
    MSG = 0x400,
    ERR = 0x008,
    HUP = 0x010,
    RDHUP = 0x2000,
    EXCLUSIVE = 1u << 28,
    WAKEUP = 1u << 29,
    ONESHOT = 1u << 30,
    ET = 1u << 31
};

enum FLAGS
{
    NORMAL,
    POSTED
};

class Multiplexer
{
  public:
    virtual ~Multiplexer(){};
    // ctx is Connection*
    virtual bool addFd(int fd, EVENTS events, void *ctx) = 0;
    // ctx is Connection*
    virtual bool modFd(int fd, EVENTS events, void *ctx) = 0;
    virtual bool delFd(int fd) = 0;
    virtual int processEvents(int flags, int timeoutMs) = 0;
    virtual void processPostedAcceptEvents() = 0;
    virtual void processPostedEvents() = 0;
};

uint32_t events2epoll(EVENTS events);
short events2poll(EVENTS events);

// @param ev Event *
int setEventTimeout(void *ev);

#endif