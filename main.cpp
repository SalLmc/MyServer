#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/global.h"
#include "src/http/http.h"
#include "src/util/utils_declaration.h"

#include <memory>

int main(int argc, char *argv[])
{
    std::unique_ptr<Cycle> cycle(new Cycle(&pool, new Logger("log/", "startup", 1)));
    cyclePtr = cycle.get();

    if (getOption(argc, argv, &mp) == -1)
    {
        LOG_CRIT << "get option failed";
        return 1;
    }

    if (init_signals() == -1)
    {
        LOG_CRIT << "init signals failed";
        return 1;
    }

    if (mp.count("signal"))
    {
        std::string signal = mp["signal"];
        pid_t pid = readPidFromFile();
        if (pid != -1)
        {
            int ret = send_signal(pid, signal);
            if (ret == 0)
            {
                return 0;
            }
            else if (ret == -1)
            {
                LOG_WARN << "Process has been stopped or send signal failed";
            }
            else if (ret == -2)
            {
                LOG_WARN << "invalid signal command";
            }
            return 0;
        }
        else
        {
            LOG_CRIT << "open pid file failed";
            return 1;
        }
    }

    if (writePid2File() == -1)
    {
        LOG_CRIT << "write pid failed";
        return 1;
    }

    if (initListen(cycle.get(), 8088) == -1)
    {
        LOG_CRIT << "init listen failed";
        return 1;
    }

    // accept mutex
    if (useAcceptMutex)
    {
        if (shmForAMtx.createShared(sizeof(ProcessMutexShare)) == -1)
        {
            LOG_CRIT << "create shm for acceptmutex failed";
            return 1;
        }
        if (shmtxCreate(&acceptMutex, (ProcessMutexShare *)shmForAMtx.getAddr()) == -1)
        {
            LOG_CRIT << "create acceptmutex failed";
            return 1;
        }
    }

    masterProcessCycle(cycle.get());
}

