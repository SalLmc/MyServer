#include "process.h"
#include "../core/core.h"
#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../log/logger.h"
#include "../util/utils_declaration.h"

extern Epoller epoller;
extern Cycle *cyclePtr;
extern ProcessMutex acceptMutex;
Process processes[MAX_PROCESS_N];

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
    isChild = 0;
    startWorkerProcesses(cycle, 2);
    if (isChild)
    {
        return;
    }

    cycle->logger_ = new Logger("log/", "master_loop", 1);

    LOG_INFO << "Looping";
    for (;;)
    {
        sigsuspend(&set);

        if (quit)
        {
            signalWorkerProcesses(SIGINT);
            break;
        }
        if (restart)
        {
            LOG_INFO << "Restart";
            signalWorkerProcesses(SIGUSR1);
        }
    }
    LOG_INFO << "Quit";
}

void startWorkerProcesses(Cycle *cycle, int n)
{
    for (int i = 0; i < n; i++)
    {
        spawnProcesses(cycle, workerProcessCycle);
    }
}

pid_t spawnProcesses(Cycle *cycle, std::function<void(Cycle *)> proc)
{
    if (processes[slot].status == ACTIVE)
    {
        slot = (slot + 1) % MAX_PROCESS_N;
        return -1;
    }

    // fd[0] holds by master process, fd[1] hold by worker process
    int fd[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) != -1);
    processes[slot].channel[0].fd_ = fd[0];
    processes[slot].channel[1].fd_ = fd[1];

    setnonblocking(fd[0]);
    setnonblocking(fd[1]);

    pid_t pid = fork();

    switch (pid)
    {
    case 0: // worker
        isChild = 1;
        processes[slot].pid = getpid();
        processes[slot].status = ACTIVE;

        // processes[slot].channel[1].read_.c = &processes[slot].channel[1];
        // processes[slot].channel[1].read_.c->read_.handler = recvFromMaster;
        // epoller.addFd(processes[slot].channel[1].fd_.getFd(), EPOLLIN, &processes[slot].channel[1]);

        proc(cycle);
        break;

    case -1:
        assert(1 == 0);
        break;

    default: // master
        processes[slot].pid = pid;
        processes[slot].status = ACTIVE;

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
        if (processes[i].status == ACTIVE)
        {
            kill(processes[i].pid, sig);
        }
    }
}

void workerProcessCycle(Cycle *cycle)
{
    // log
    char name[20];
    sprintf(name, "worker_loop_%d", getpid());
    cycle->logger_ = new Logger("log/", name, 1);

    // sig
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    {
        LOG_CRIT << "sigprocmask failed";
        exit(1);
    }

    // epoll
    epoller.setEpollFd(epoll_create(5));

    // cycle->timer_.Add(
    //     1, getTickMs(),
    //     [&](void *arg) {
    //         printf("%d timer msg:%s\n", getpid(), (char *)arg);
    //         cycle->timer.Again(1, getTickMs() + 1000);
    //         return 1;
    //     },
    //     (void *)"HELLO");

    LOG_INFO << "Worker Looping";
    for (;;)
    {
        processEventsAndTimers(cycle);

        if (quit)
        {
            break;
        }
        if (restart)
        {
            LOG_INFO << "Worker Restart";
        }
    }

    LOG_INFO << "Worker Quit";
}

void processEventsAndTimers(Cycle *cycle)
{
    if (useAcceptMutex)
    {
        if (acceptexTryLock(cycle) == -1)
        {
            return;
        }
    }

    epoller.processEvents(0, 0);

    if (acceptMutexHeld)
    {
        shmtxUnlock(&acceptMutex);
    }

    cycle->timer_.Tick();
}