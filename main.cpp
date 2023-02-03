#include "src/core/core.h"
#include "src/event/epoller.h"
#include "src/http/http.h"
#include "src/util/utils_declaration.h"
#include "src/global.h"
#include "src/core/process.h"

#include <memory>

int main(int argc, char *argv[])
{
    nanolog::initialize(nanolog::GuaranteedLogger(), "log/", "log", 1);

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
            if (send_signal(pid, signal) == -1)
            {
                if (pid != -1)
                {
                    LOG_INFO << "process has been stopped";
                    return 0;
                }
                else
                {
                    LOG_CRIT << "send signal failed";
                    return 1;
                }
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
        return -1;
    }

    std::unique_ptr<Cycle> cycle(new Cycle());

    masterProcessCycle(cycle.get());
}