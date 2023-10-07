#ifndef UTILS_DECLARATION_H
#define UTILS_DECLARATION_H

#include "../headers.h"

#define asm_pause() __asm__("pause")

/* utils.cpp */

int setnonblocking(int fd);
unsigned long long getTickMs();
int getOption(int argc, char *argv[], std::unordered_map<std::string, std::string> *mp);
int writePid2File();
std::string mtime2str(timespec *mtime);
std::string byte2properstr(off_t bytes);
std::pair<std::string, int> getServer(std::string addr);
std::string getLeftUri(std::string addr);
unsigned char ToHex(unsigned char x);
unsigned char FromHex(unsigned char x);
// space to %20
std::string UrlEncode(const std::string &str, char ignore);
// %20 to space
std::string UrlDecode(const std::string &str);
std::string fd2Path(int fd);
bool isMatch(std::string src, std::string pattern);
void mkdir_r(const char *path, mode_t mode);

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

template <class T> T readNumberFromFile(const char *file_name)
{
    int filefd(open(file_name, O_RDONLY));
    if (filefd < 0)
    {
        return -1;
    }
    char buffer[100];
    memset(buffer, '\0', sizeof(buffer));
    if (read(filefd, buffer, sizeof(buffer) - 1) <= 0)
    {
        close(filefd);
        return -1;
    }
    close(filefd);
    T num = atoi(buffer);
    return num;
}

#endif