#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/event/epoller.h"
#include "../src/http/http.h"
#include "../src/utils/utils.h"
#include "../src/global.h"
#include "../src/log/logger.h"

Level loggerLevel = Level::INFO;
class Server;
bool enableLogger;
Server *serverPtr;

std::unordered_map<Arg, std::string> mp;

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

    if (mp.count(Arg::SIGNAL))
    {
        std::string signal = mp[Arg::SIGNAL];
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