#include "../headers.h"

#include "../core/core.h"
#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../http/http.h"
#include "../log/logger.h"
#include "../utils/utils_declaration.h"
#include "process.h"

#include "../memory/memory_manage.hpp"

extern Cycle *cyclePtr;
extern ProcessMutex acceptMutex;
extern HeapMemory heap;
extern std::list<Event *> posted_accept_events;
extern std::list<Event *> posted_events;
extern int processes;

Process procs[MAX_PROCESS_N];

void masterProcessCycle(Cycle *cycle)
{
    // signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        LOG_CRIT << "sigprocmask failed";
        return;
    }

    sigemptyset(&set);

    // logger
    if (cycle->logger_ != NULL)
    {
        delete cycle->logger_;
        cycle->logger_ = NULL;
    }

    // start processes
    if (only_worker)
    {
        workerProcessCycle(cycle);
        return;
    }

    isChild = 0;

    startWorkerProcesses(cycle, processes);

    if (isChild)
    {
        return;
    }

    // logger
    cycle->logger_ = new Logger("log/", "master");
    LOG_INFO << "Looping";
    for (;;)
    {
        sigsuspend(&set);

        if (quit)
        {
            signalWorkerProcesses(SIGINT);
            break;
        }
    }
    LOG_INFO << "Quit";
}

void startWorkerProcesses(Cycle *cycle, int n)
{
    int num = std::min(n, MAX_PROCESS_N);
    for (int i = 0; i < num && !isChild; i++)
    {
        spawnProcesses(cycle, workerProcessCycle);
    }
}

pid_t spawnProcesses(Cycle *cycle, std::function<void(Cycle *)> proc)
{
    pid_t pid = fork();

    switch (pid)
    {
    case 0: // worker
        isChild = 1;
        procs[slot].pid = getpid();
        procs[slot].status = ACTIVE;

        proc(cycle);
        break;

    case -1:
        LOG_WARN << "fork error";
        break;

    default: // master
        procs[slot].pid = pid;
        procs[slot].status = ACTIVE;

        slot++;
        break;
    }
    return pid;
}

int recvFromMaster(Event *rev)
{
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    recv(rev->c->fd_.getFd(), buffer, 63, 0);
    printf("slot:%d, recv from worker:%s\n", slot, buffer);
    return 0;
}

void signalWorkerProcesses(int sig)
{
    for (int i = 0; i < MAX_PROCESS_N; i++)
    {
        if (procs[i].status == ACTIVE)
        {
            if (kill(procs[i].pid, sig) == -1)
            {
                LOG_CRIT << "Send signals to " << procs[i].pid << " failed";
            }
        }
    }
}

void workerProcessCycle(Cycle *cycle)
{
    // log
    char name[20];
    sprintf(name, "worker_%d", slot);
    cycle->logger_ = new Logger("log/", name);

    // sig
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    {
        LOG_CRIT << "sigprocmask failed";
        exit(1);
    }

    // listen
    for (auto &x : cyclePtr->servers_)
    {
        if (initListen(cyclePtr, x.port) == ERROR)
        {
            LOG_CRIT << "init listen failed";
        }
    }

    // epoll
    Epoller *epoller = dynamic_cast<Epoller *>(cyclePtr->multiplexer);
    if (epoller)
    {
        epoller->setEpollFd(epoll_create1(0));
    }

    for (auto &listen : cycle->listening_)
    {
        // use LT on listenfd
        if (cyclePtr->multiplexer->addFd(listen->fd_.getFd(), EVENTS(IN), listen) == 0)
        {
            LOG_CRIT << "Listenfd add failed, errno:" << strerror(errno);
        }
    }

    // timer
    if (enable_logger)
    {
        cyclePtr->timer_.Add(-1, getTickMs() + 3000, logging, (void *)3000);
    }

    LOG_INFO << "Worker Looping";
    for (;;)
    {
        processEventsAndTimers(cycle);

        if (quit)
        {
            break;
        }
    }

    LOG_INFO << "Worker Quit";
}

void processEventsAndTimers(Cycle *cycle)
{
    int flags = 0;

    unsigned long long nextTick = cycle->timer_.GetNextTick();
    nextTick = ((nextTick == (unsigned long long)-1) ? -1 : (nextTick - getTickMs()));

    int ret = cyclePtr->multiplexer->processEvents(flags, nextTick);
    if (ret == -1)
    {
        LOG_WARN << "process events errno: " << strerror(errno);
    }

    process_posted_events(&posted_accept_events);

    cycle->timer_.Tick();

    process_posted_events(&posted_events);
}

int logging(void *arg)
{
    cyclePtr->logger_->wakeup();
    cyclePtr->timer_.Again(-1, getTickMs() + (long long)arg);
    return 0;
}