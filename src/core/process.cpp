#include "../headers.h"

#include "../core/core.h"
#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../http/http.h"
#include "../log/logger.h"
#include "../utils/utils.h"
#include "process.h"

extern Server *serverPtr;

Process procs[MAX_PROCESS_N];

void masterProcessCycle(Server *server)
{
    // signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SERVER_STOP);

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
    if (serverConfig.onlyWorker)
    {
        workerProcessCycle(server);
        return;
    }

    isChild = 0;

    startWorkerProcesses(server, serverConfig.processes);

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
            signalWorkerProcesses(SERVER_STOP);
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
    server->logger_->threshold_ = serverConfig.loggerThreshold;

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
    if (server->initListen(newConnection) == ERROR)
    {
        LOG_CRIT << "init listen failed";
    }

    server->initEvent(serverConfig.useEpoll);

    if (server->regisListen() == ERROR)
    {
        LOG_CRIT << "Listenfd add failed, errno:" << strerror(errno);
    }

    // timer
    if (enable_logger)
    {
        unsigned long long interval = serverConfig.loggerInterval * 1000;
        server->timer_.add(LOG, "logger timer", getTickMs() + interval, logging, (void *)interval);
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