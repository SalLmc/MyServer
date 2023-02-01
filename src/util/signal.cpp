#include "../core/core.h"
#include "utils_declaration.h"
#include "../global.h"

void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        quit = 1;
        break; /*  */
    case SIGUSR1:
        restart = 1;
        break;
    }
}

int init_signals()
{
    for (int i = 0; signals[i].command != ""; i++)
    {
        struct sigaction sa;
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = signals[i].handler;
        sa.sa_flags |= SA_RESTART;
        sigemptyset(&sa.sa_mask);
        if (sigaction(signals[i].sig, &sa, NULL) == -1)
        {
            return -1;
        }
    }
    return 0;
}

int send_signal(pid_t pid, std::string command)
{
    for (int i = 0; signals[i].command != ""; i++)
    {
        if (signals[i].command == command)
        {
            if (kill(pid, signals[i].sig) != -1)
            {
                return 0;
            }
        }
    }
    return -1;
}