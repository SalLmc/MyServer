#include "../headers.h"

#include "../core/core.h"
#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../http/http.h"
#include "../log/logger.h"
#include "../utils/utils_declaration.h"
#include "process.h"

extern Server *serverPtr;
extern ProcessMutex acceptMutex;
extern int processes;

Process procs[MAX_PROCESS_N];

void masterProcessCycle(Server *server)
{
    // signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        LOG_CRIT << "sigprocmask failed";
        return;
    }

    sigemptyset(&set);

    // logger
    if (server->logger_ != NULL)
    {
        delete server->logger_;
        server->logger_ = NULL;
    }

    // start processes
    if (only_worker)
    {
        workerProcessCycle(server);
        return;
    }

    isChild = 0;

    startWorkerProcesses(server, processes);

    if (isChild)
    {
        return;
    }

    // logger
    server->logger_ = new Logger("log/", "master");
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

void startWorkerProcesses(Server *server, int n)
{
    int num = std::min(n, MAX_PROCESS_N);
    for (int i = 0; i < num && !isChild; i++)
    {
        spawnProcesses(server, workerProcessCycle);
    }
}

pid_t spawnProcesses(Server *server, std::function<void(Server *)> proc)
{
    pid_t pid = fork();

    switch (pid)
    {
    case 0: // worker
        isChild = 1;
        procs[slot].pid_ = getpid();
        procs[slot].status_ = ProcessStatus::ACTIVE;

        proc(server);
        break;

    case -1:
        LOG_WARN << "fork error";
        break;

    default: // master
        procs[slot].pid_ = pid;
        procs[slot].status_ = ProcessStatus::ACTIVE;

        slot++;
        break;
    }
    return pid;
}

int recvFromMaster(Event *rev)
{
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    recv(rev->c_->fd_.getFd(), buffer, 63, 0);
    printf("slot:%d, recv from worker:%s\n", slot, buffer);
    return 0;
}

void signalWorkerProcesses(int sig)
{
    for (int i = 0; i < MAX_PROCESS_N; i++)
    {
        if (procs[i].status_ == ProcessStatus::ACTIVE)
        {
            if (kill(procs[i].pid_, sig) == -1)
            {
                LOG_CRIT << "Send signals to " << procs[i].pid_ << " failed";
            }
        }
    }
}

void workerProcessCycle(Server *server)
{
    // log
    char name[20];
    sprintf(name, "worker_%d", slot);
    server->logger_ = new Logger("log/", name);
    server->logger_->threshold_ = logger_threshold;

    // sig
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    {
        LOG_CRIT << "sigprocmask failed";
        exit(1);
    }

    // log fd
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
    {
        LOG_INFO << "max fd: " << rlim.rlim_max;
    }

    // listen
    for (auto &x : server->servers_)
    {
        if (initListen(server, x.port_) == ERROR)
        {
            LOG_CRIT << "init listen failed";
        }
    }

    // epoll
    Epoller *epoller = dynamic_cast<Epoller *>(server->multiplexer_);
    if (epoller)
    {
        epoller->setEpollFd(epoll_create1(0));
    }

    for (auto &listen : server->listening_)
    {
        // use LT on listenfd
        if (server->multiplexer_->addFd(listen->fd_.getFd(), EVENTS(IN), listen) == 0)
        {
            LOG_CRIT << "Listenfd add failed, errno:" << strerror(errno);
        }
    }

    // timer
    if (enable_logger)
    {
        server->timer_.add(LOG, "logger timer", getTickMs() + 3000, logging, (void *)3000);
    }

    LOG_INFO << "Worker Looping";
    for (;;)
    {
        server->eventLoop();

        if (quit)
        {
            break;
        }
    }

    LOG_INFO << "Worker Quit";
}

int logging(void *arg)
{
    serverPtr->logger_->wakeup();
    serverPtr->timer_.again(-1, getTickMs() + (long long)arg);
    return 0;
}