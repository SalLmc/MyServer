#include "process.h"
#include "../global.h"

void masterProcessCycle(Cycle *cycle)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        // LOG_CRIT << "sigprocmask failed";
        return;
    }

    sigemptyset(&set);

    startWorkerProcesses(cycle, 2);

    printf("looping\n");
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
            printf("restart\n");
        }
    }
    printf("quit\n");
}

void startWorkerProcesses(Cycle *cycle, int n)
{
    for (int i = 0; i < n; i++)
    {
        spawnProcesses(cycle, workerProcessCycle);
    }
}

void workerProcessCycle(Cycle *cycle)
{
    sigset_t set;
    sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    {
        // LOG_CRIT << "sigprocmask failed";
        exit(1);
    }

    printf("worker looping\n");
    for (;;)
    {
        epoller.processEvents();

        if (quit)
        {
            break;
        }
        if (restart)
        {
            printf("worker restart\n");
        }
    }

    printf("worker quit\n");
    exit(0);
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
        processes[slot].pid = getpid();
        processes[slot].status = ACTIVE;

        processes[slot].channel[1].read_.c = &processes[slot].channel[1];
        processes[slot].channel[1].read_.c->read_.handler = recvFromMaster;

        epoller.addFd(processes[slot].channel[1].fd_.getFd(), EPOLLIN, &processes[slot].channel[1]);

        proc(cycle);

        break;
    case -1:
        // LOG_CRIT << "fork() failed";
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

// int recvFromWorker(Event *rev)
// {
//     char buffer[64];
//     memset(buffer, 0, sizeof(buffer));
//     recv(rev->c->fd_.getFd(), buffer, 63, 0);

//     // printf("slot:%d, recv from worker:%d\n", slot, idx);
//     LOG_INFO<<"slot:"<<slot<<", recv from worker:"<<buffer;
//     return 0;
// }

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