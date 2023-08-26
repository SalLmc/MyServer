#include "../headers.h"

#include "../global.h"
#include "utils_declaration.h"

SignalWrapper signals[] = {
    {SIGINT, signal_handler, "stop"}, {SIGUSR1, signal_handler, "reload"}, {SIGPIPE, SIG_IGN, ""}, {0, NULL, ""}};

void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        quit = 1;
        break;
    case SIGUSR1:
        restart = 1;
        break;
    }
}

int init_signals()
{
    for (int i = 0; signals[i].sig != 0; i++)
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
    int sent = 0;
    for (int i = 0; signals[i].sig != 0; i++)
    {
        if (signals[i].command == command)
        {
            sent = 1;
            if (kill(pid, signals[i].sig) != -1)
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