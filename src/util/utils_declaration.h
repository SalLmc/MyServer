#ifndef UTILS_DECLARATION_H
#define UTILS_DECLARATION_H

#include "../core/core.h"
#include "../global.h"
#include <signal.h>
#include <string>
#include <unordered_map>

#define asm_pause() __asm__("pause")

/* utils.cpp */

int setnonblocking(int fd);
unsigned long long getTickMs();
// return 0 for success, -1 for invalid option
int getOption(int argc, char *argv[], std::unordered_map<std::string, std::string> *mp);

// return 0 for success, -1 for failure
int writePid2File();
// return pid for success, -1 for file not existed
pid_t readPidFromFile();

/* signal.cpp */

struct SignalWrapper
{
    int sig;
    __sighandler_t handler;
    std::string command;
};
void signal_handler(int sig);
// return 0 for success, -1 for failure
int init_signals();
// return 0 for success, -1 for failure
int send_signal(pid_t pid, std::string command);

#endif