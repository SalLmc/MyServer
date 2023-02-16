#include "utils_declaration.h"
#include "../core/core.h"

int setnonblocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

unsigned long long getTickMs()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    unsigned long long u = now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
}

int getOption(int argc, char *argv[], std::unordered_map<std::string, std::string> *mp)
{
    char *now;
    for (int i = 1; i < argc; i++)
    {
        now = (char *)argv[i];
        if (*now != '-')
        {
            goto failed;
        }
        now++;
        switch (*now)
        {
        case 'p':
            i++;
            if (!argv[i])
                goto failed;
            mp->insert(std::make_pair<std::string, std::string>("port", std::string(argv[i])));
            break;
        case 's':
            i++;
            if (!argv[i])
                goto failed;
            mp->insert(std::make_pair<std::string, std::string>("signal", std::string(argv[i])));
            break;
        }
    }
    return OK;

failed:
    return ERROR;
}

int writePid2File()
{
    Fd filefd = open("pid_file", O_CREAT | O_RDWR, 0666);
    if (filefd.getFd() < 0)
    {
        return ERROR;
    }

    pid_t pid = getpid();
    char buffer[100];
    memset(buffer, '\0', sizeof(buffer));
    int len = sprintf(buffer, "%d\n", pid);
    write(filefd.getFd(), buffer, len);
    return OK;
}

pid_t readPidFromFile()
{
    Fd filefd = open("pid_file", O_RDWR);
    if (filefd.getFd() < 0)
    {
        return ERROR;
    }
    char buffer[100];
    memset(buffer, '\0', sizeof(buffer));
    assert(read(filefd.getFd(), buffer, sizeof(buffer) - 1) > 0);
    pid_t pid = atoi(buffer);
    return pid;
}