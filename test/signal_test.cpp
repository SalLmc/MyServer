#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/event/epoller.h"
#include "../src/http/http.h"
#include "../src/utils/utils.h"
#include "../src/global.h"

class Server;
bool enable_logger;
Server *serverPtr;

std::unordered_map<std::string, std::string> mp;

int main(int argc, char *argv[])
{
    assert(initSignals() == 0);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);

    assert(sigprocmask(SIG_BLOCK, &set, NULL) != -1);
    sigemptyset(&set);

    assert(getOption(argc, argv, &mp) == 0);

    if (mp.count("signal"))
    {
        std::string signal = mp["signal"];
        pid_t pid = readNumberFromFile<pid_t>("pid_file");
        if (pid != -1)
        {
            assert(sendSignal(pid, signal) == 0);
            return 0;
        }
    }

    writePid2File();

    printf("looping\n");
    for (;;)
    {
        sigsuspend(&set);

        if (quit)
        {
            break;
        }
    }
    printf("quit\n");
}