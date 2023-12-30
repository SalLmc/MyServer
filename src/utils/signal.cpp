#include "../headers.h"

#include "../global.h"
#include "utils.h"

std::vector<SignalWrapper> signals{{SIGINT, signalHandler, "stop"}, {SIGPIPE, SIG_IGN, ""}};

void signalHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        quit = 1;
        break;
    default:
        break;
    }
}

int initSignals()
{
    for (auto &x : signals)
    {
        struct sigaction sa;
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = x.handler;
        sa.sa_flags |= SA_RESTART;
        sigemptyset(&sa.sa_mask);
        if (sigaction(x.sig, &sa, NULL) == -1)
        {
            return -1;
        }
    }
    return 0;
}

int sendSignal(pid_t pid, std::string command)
{
    int sent = 0;
    for (auto &x : signals)
    {
        if (x.command == command)
        {
            sent = 1;
            if (kill(pid, x.sig) != -1)
            {
                return 0;
            }
        }
    }
    if (sent)
        return -1;
    else
        return -2;
}