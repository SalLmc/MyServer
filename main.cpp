#include "src/headers.h"

#include "src/core/core.h"
#include "src/core/process.h"
#include "src/event/epoller.h"
#include "src/event/event.h"
#include "src/global.h"
#include "src/http/http.h"
#include "src/util/utils_declaration.h"

std::unordered_map<std::string, std::string> mp;
extern ConnectionPool cPool;
extern Cycle *cyclePtr;
extern sharedMemory shmForAMtx;
extern ProcessMutex acceptMutex;
extern Epoller epoller;

int main(int argc, char *argv[])
{
    std::unique_ptr<Cycle> cycle(new Cycle(&cPool, new Logger("log/", "startup", 1)));
    cyclePtr = cycle.get();

    if (getOption(argc, argv, &mp) == ERROR)
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

    if (writePid2File() == ERROR)
    {
        LOG_CRIT << "write pid failed";
        return 1;
    }

    // server init
    // port root index from to auto_index try_files
    cyclePtr->servers_.emplace_back(8081, "/home/sallmc/dist", "index.html", "/api/", "http://175.178.175.106:8080/", 0,
                                    std::vector<std::string>{"index.html"});
    cyclePtr->servers_.emplace_back(8082, "/home/sallmc/share", "sdfxcv", "", "", 1, std::vector<std::string>{});
    // cyclePtr->servers_.emplace_back(8083, "static", "index.html", "", "", 0, std::vector<std::string>{"index.html"});

    // for (auto &x : cyclePtr->servers_)
    // {
    //     if (initListen(cyclePtr, x.port) == ERROR)
    //     {
    //         LOG_CRIT << "init listen failed";
    //         return 1;
    //     }
    // }

    // accept mutex
    if (useAcceptMutex)
    {
        if (shmForAMtx.createShared(sizeof(ProcessMutexShare)) == ERROR)
        {
            LOG_CRIT << "create shm for acceptmutex failed";
            return 1;
        }

        ProcessMutexShare *share = (ProcessMutexShare *)shmForAMtx.getAddr();
        share->lock = 0;
        share->wait = 0;

        if (shmtxCreate(&acceptMutex, (ProcessMutexShare *)shmForAMtx.getAddr()) == ERROR)
        {
            LOG_CRIT << "create acceptmutex failed";
            return 1;
        }
    }

    masterProcessCycle(cyclePtr);
}
