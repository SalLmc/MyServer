#include "../headers.h"

#include "../core/basic.h"
#include "utils_declaration.h"

int setNonblocking(int fd)
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
    Fd filefd(open("pid_file", O_CREAT | O_RDWR | O_TRUNC, 0666));
    if (filefd.getFd() < 0)
    {
        return ERROR;
    }

    pid_t pid = getpid();
    char buffer[100];
    memset(buffer, '\0', sizeof(buffer));
    int len = sprintf(buffer, "%d", pid);
    write(filefd.getFd(), buffer, len);
    return OK;
}

std::string mtime2Str(timespec *mtime)
{
    char buf[21];
    buf[20] = '\0';
    struct tm *tm;
    tm = localtime(&mtime->tv_sec);
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf);
}

std::string byte2ProperStr(off_t bytes)
{
    double K = 1024;
    double M = 1024 * K;
    double G = 1024 * M;

    double ans;
    std::ostringstream stream;

    if (bytes < K)
    {
        return std::to_string(bytes);
    }
    else if (bytes < M)
    {
        ans = bytes / K;
        stream << std::fixed << std::setprecision(1) << ans;
        return stream.str() + "K";
    }
    else if (bytes < G)
    {
        ans = bytes / M;
        stream << std::fixed << std::setprecision(1) << ans;
        return stream.str() + "M";
    }
    else
    {
        ans = bytes / G;
        stream << std::fixed << std::setprecision(1) << ans;
        return stream.str() + "G";
    }
}

/// @brief get ip/hostname and port from a URI
/// @param addr
/// @return < [ipLeft, ipRight], [portLeft, portRight] >. if addr doesn't have port, then portLeft == ipLeft &&
/// portRight == ipRight
static std::pair<std::pair<int, int>, std::pair<int, int>> getServerInner(std::string addr)
{
    // [l, r]
    int ipl = 0, ipr = 0;
    int pl = 0, pr = 0;
    bool hasPort = 0;
    enum class State
    {
        PROTOCOL,
        COLON0,
        SLASH0,
        SLASH1,
        ADDR,
        COLON1,
        PORT,
        END
    };
    State st = State::PROTOCOL;
    for (decltype(addr.length()) i = 0; i < addr.length() && st != State::END;)
    {
        char now = addr[i];
        switch (st)
        {
        case State::PROTOCOL:
            if ('a' <= now && now <= 'z')
            {
                i++;
            }
            else if (now == ':')
            {
                st = State::COLON0;
            }
            else
            {
                throw std::runtime_error("Error when parsing protocol");
            }
            break;
        case State::COLON0:
            if (now == ':')
            {
                st = State::SLASH0;
                i++;
            }
            else
            {
                throw std::runtime_error("Error when parsing colon0");
            }
            break;
        case State::SLASH0:
            if (now == '/')
            {
                st = State::SLASH1;
                i++;
            }
            else
            {
                throw std::runtime_error("Error when parsing stash0");
            }
            break;
        case State::SLASH1:
            if (now == '/')
            {
                st = State::ADDR;
                i++;
                ipl = i;
            }
            else
            {
                throw std::runtime_error("Error when parsing stash1");
            }
            break;
        case State::ADDR:
            if (now == ':')
            {
                st = State::COLON1;
                ipr = i - 1;
            }
            else if (now == '/')
            {
                st = State::END;
                ipr = i - 1;
                i++;
            }
            else if (i == addr.length() - 1)
            {
                st = State::END;
                ipr = i;
                i++;
            }
            else
            {
                i++;
            }
            break;
        case State::COLON1:
            if (now == ':')
            {
                st = State::PORT;
                i++;
                pl = i;
            }
            else
            {
                throw std::runtime_error("Error when parsing colon1");
            }
            break;
        case State::PORT:
            if ('0' <= now && now <= '9')
            {
                if (i == addr.length() - 1)
                {
                    st = State::END;
                    hasPort = 1;
                    pr = i;
                    i++;
                }
                else
                {
                    i++;
                }
            }
            else if (now == '/')
            {
                st = State::END;
                hasPort = 1;
                pr = i - 1;
                i++;
            }
            else
            {
                throw std::runtime_error("Error when parsing port");
            }
            break;
        case State::END:
            break;
        default:
            break;
        }
    }

    if (ipl <= ipr && pl <= pr && st == State::END)
    {
        if (!hasPort)
        {
            pl = ipl;
            pr = ipr;
        }
        return std::make_pair(std::make_pair(ipl, ipr), std::make_pair(pl, pr));
    }
    else
    {
        throw std::runtime_error("Wrong format");
    }
}

std::pair<std::string, int> getServer(std::string addr)
{
    auto ans = getServerInner(addr);

    std::string ip = "";
    int port = 80;

    bool hasPort = 1;
    if (ans.first.first == ans.second.first && ans.first.second == ans.second.second)
    {
        hasPort = 0;
    }

    ip = addr.substr(ans.first.first, ans.first.second - ans.first.first + 1);

    if (hasPort)
    {
        std::string sPort = addr.substr(ans.second.first, ans.second.second - ans.second.first + 1);
        port = std::stoi(sPort);
    }

    return std::make_pair(ip, port);
}

std::string getLeftUri(std::string addr)
{
    auto ans = getServerInner(addr);
    decltype(addr.length()) left = ans.second.second + 1;
    if (left == addr.length())
    {
        return "/";
    }
    else
    {
        return addr.substr(left, addr.length() - left + 1);
    }
}

unsigned char toHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

unsigned char fromHex(unsigned char x)
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
        return x;
    }
    return y;
}

std::string urlEncode(const std::string &str, char ignore)
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
            strTemp += toHex((unsigned char)str[i] >> 4);
            strTemp += toHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string urlDecode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '%')
        {
            if (i + 2 < length)
            {
                unsigned char high = fromHex((unsigned char)str[++i]);
                unsigned char low = fromHex((unsigned char)str[++i]);
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

void recursiveMkdir(const char *path, mode_t mode)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, mode);
            *p = '/';
        }
    mkdir(tmp, mode);
}

std::string getIpByDomain(std::string &domain)
{
    hostent *host_info = gethostbyname2(domain.c_str(), AF_INET);
    if (host_info == NULL)
    {
        throw std::runtime_error("Get host by name failed");
    }
    struct in_addr ipv4;
    ipv4.s_addr = *(unsigned int *)host_info->h_addr_list[0];
    return std::string(inet_ntoa(ipv4));
}

bool isHostname(const std::string &str)
{
    std::regex hostnameRegex("^[a-zA-Z0-9.-]+");
    return std::regex_match(str, hostnameRegex);
}

bool isIPAddress(const std::string &str)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str.c_str(), &(sa.sin_addr)) != 0;
}

std::string toLower(std::string &src)
{
    std::string ans;
    for (auto &c : src)
    {
        ans += tolower(c);
    }
    return ans;
}