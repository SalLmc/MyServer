#ifndef UTILS_DECLARATION_H
#define UTILS_DECLARATION_H

#include <signal.h>
#include <string>
#include <unordered_map>

#define asm_pause() __asm__("pause")

/* utils.cpp */

int setnonblocking(int fd);
unsigned long long getTickMs();
int getOption(int argc, char *argv[], std::unordered_map<std::string, std::string> *mp);
int writePid2File();
pid_t readPidFromFile();
std::string mtime2str(timespec *mtime);
std::string byte2properstr(off_t bytes);

unsigned char ToHex(unsigned char x);
unsigned char FromHex(unsigned char x);
std::string UrlEncode(const std::string &str);
std::string UrlDecode(const std::string &str);

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
// return 0 for success, -1 for failure, -2 for invalid command
int send_signal(pid_t pid, std::string command);

#endif