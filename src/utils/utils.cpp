#include "../headers.h"

#include "../core/core.h"
#include "utils_declaration.h"

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
    Fd filefd(open("pid_file", O_CREAT | O_RDWR, 0666));
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

std::string mtime2str(timespec *mtime)
{
    char buf[21];
    buf[20] = '\0';
    struct tm *tm;
    tm = localtime(&mtime->tv_sec);
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf);
}

std::string byte2properstr(off_t bytes)
{
    int K = 1024;
    int M = 1024 * K;
    int G = 1024 * M;

    if (bytes < K)
    {
        return std::to_string(bytes);
    }
    else if (bytes < M)
    {
        return std::to_string(bytes / K + (bytes % K != 0)) + "K";
    }
    else if (bytes < G)
    {
        return std::to_string(bytes / M + (bytes % M != 0)) + "M";
    }
    else
    {
        return std::to_string(bytes / G + (bytes % G != 0)) + "G";
    }
}

// @param addr is like http://xxx.xxx.xxx.xx/ or https://xxx.xxx.xxx.xx:8080/
std::string getIp(std::string addr)
{
    auto first = addr.find_first_of(':');
    auto last = addr.find_last_of(':');
    if (first == last)
    {
        return addr.substr(first + 3, addr.length() - 1 - first - 3);
    }
    return addr.substr(first + 3, last - first - 3);
}
// @param addr is like http://xxx.xxx.xxx.xx/ or https://xxx.xxx.xxx.xx:8080/
int getPort(std::string addr)
{
    auto first = addr.find_first_of(':');
    auto last = addr.find_last_of(':');
    if (first == last)
    {
        return 80;
    }
    return std::stoi(addr.substr(last + 1, addr.length() - 1 - last - 1));
}
// @param addr is like http://xxx.xxx.xxx.xx/ or https://xxx.xxx.xxx.xx:8080/ttt/
std::string getNewUri(std::string addr)
{
    auto first = addr.find_first_of('/', 8);
    return addr.substr(first, addr.length() - first);
}

unsigned char ToHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
    {
        y = x - 'A' + 10;
    }
    else if (x >= 'a' && x <= 'z')
    {
        y = x - 'a' + 10;
    }
    else if (x >= '0' && x <= '9')
    {
        y = x - '0';
    }
    else
    {
        // assert(0);
    }
    return y;
}

std::string UrlEncode(const std::string &str, char ignore)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) || (str[i] == ignore) || (str[i] == '-') || (str[i] == '_') ||
            (str[i] == '.') || (str[i] == '~'))
            strTemp += str[i];
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '%')
        {
            if (i + 2 < length)
            {
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
        }
        else
            strTemp += str[i];
    }
    return strTemp;
}

std::string fd2Path(int fd)
{
    if (fd < 0)
        return "";
    char path[32];
    memset(path, 0, sizeof(path));
    sprintf(path, "/proc/self/fd/%d", fd);
    char buf[100];
    memset(buf, 0, sizeof(buf));
    int ret = readlink(path, buf, sizeof(buf) - 1);
    if (ret == -1)
        return "";
    std::string s(buf);
    return s;
}

bool isMatch(std::string src, std::string pattern)
{
    int n = src.size();
    int m = pattern.size();
    src = " " + src;
    pattern = " " + pattern;

    bool dp[n + 1][m + 1];
    memset(dp, 0, sizeof(dp));
    dp[0][0] = 1;

    for (int i = 1; i <= m; i++)
    {
        if (pattern[i] == '*')
        {
            dp[0][i] = 1;
        }
        else
        {
            break;
        }
    }

    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= m; j++)
        {
            if (src[i] == pattern[j] || pattern[j] == '?')
            {
                dp[i][j] = std::max(dp[i][j], dp[i - 1][j - 1]);
            }
            else if (pattern[j] == '*')
            {
                bool save = dp[i][j];
                dp[i][j] = std::max(dp[i][j - 1], dp[i - 1][j]);
                dp[i][j] = std::max(save, dp[i][j]);
            }
        }
    }

    return dp[n][m];
}