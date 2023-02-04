#include "../core/core.h"
#include "../util/utils_declaration.h"
#include "logger.h"

LogLine::LogLine()
{
    memset(buffer_, 0, sizeof(buffer_));
    pos = 0;
}

LogLine::LogLine(Level level, char const *file, char const *function, unsigned int line)
{
    // time
    timeStamp_ = getTickMs();
    auto shortTime = timeStamp_ / 1000;
    std::time_t time_t = shortTime;
    auto gmtime = std::gmtime(&time_t);
    gmtime->tm_hour += 8;
    char buffer[32];
    strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", gmtime);
    strcpy(ctimeStamp_, buffer);

    // pid
    pid_ = getpid();
    sprintf(cPid_, "%d", pid_);

    // line
    sprintf(cLine_, "%d", line);
    line_ = line;

    // level
    switch (level)
    {
    case Level::CRIT:
        cLevel_ = "CRIT";
        break;
    case Level::INFO:
        cLevel_ = "INFO";
        break;
    case Level::WARN:
        cLevel_ = "WARN";
        break;
    }
    level_ = level;

    // file
    file_ = file;

    // function
    function_ = function;
}

LogLine &LogLine::operator<<(int8_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(uint8_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(int16_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(uint16_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(int32_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(uint32_t arg)
{
    sprintf(buffer_ + pos, "%d", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(int64_t arg)
{
    sprintf(buffer_ + pos, "%ld", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(uint64_t arg)
{
    sprintf(buffer_ + pos, "%ld", arg);
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(std::string &arg)
{
    strcpy(buffer_ + pos, arg.c_str());
    pos += strlen(buffer_ + pos);
    return *this;
}
LogLine &LogLine::operator<<(const char *arg)
{
    strcpy(buffer_ + pos, arg);
    pos += strlen(buffer_ + pos);
    return *this;
}

// @param buffer output
// @param n maxsize
// save string to buffer.
// if length > n do nothing and return -1, return real length for success.
int LogLine::stringify(char *buffer, int n)
{
    char tmp[2048];
    memset(tmp, 0, sizeof(tmp));
    int pos = 0;

    // level
    tmp[pos++] = '[';
    strcpy(tmp + pos, cLevel_);
    pos += strlen(tmp + pos);
    tmp[pos++] = ']';
    tmp[pos++] = ' ';

    // time
    tmp[pos++] = '[';
    strcpy(tmp + pos, ctimeStamp_);
    pos += strlen(tmp + pos);
    tmp[pos++] = ']';
    tmp[pos++] = ' ';

    // pid
    tmp[pos++] = '<';
    strcpy(tmp + pos, cPid_);
    pos += strlen(tmp + pos);
    tmp[pos++] = '>';
    tmp[pos++] = ' ';

    // file function line
    strcpy(tmp + pos, file_);
    pos += strlen(tmp + pos);
    tmp[pos++] = ':';
    strcpy(tmp + pos, function_);
    pos += strlen(tmp + pos);
    tmp[pos++] = ':';
    strcpy(tmp + pos, cLine_);
    pos += strlen(tmp + pos);
    tmp[pos++] = ' ';

    // content
    strcpy(tmp + pos, buffer_);
    pos += strlen(tmp + pos);

    if (pos <= n)
    {
        strcpy(buffer, tmp);
        return pos;
    }
    else
    {
        return -1;
    }
}

Logger::Logger(const char *path, const char *name, unsigned long long size)
    : filePath_(path), fileName_(name), maxFileSize_(size), cnt(1), writeThread(&Logger::write2File, this),
      writing2List(0), shutdown(0)
{
    char loc[100];
    memset(loc, 0, sizeof(loc));
    sprintf(loc, "%s%s_%d", filePath_, fileName_, cnt++);
    fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
    assert(fd_.getFd() >= 0);
}
Logger::~Logger()
{
    shutdown.store(1);
    while (!writeThread.joinable())
    {
    }
    writeThread.join();
}
Logger &Logger::operator+=(LogLine &line)
{
    writing2List.store(1, std::memory_order_acquire);
    ls_.push_back(line);
    writing2List.store(0, std::memory_order_release);
    return *this;
}

void Logger::write2File()
{
    while (!shutdown.load())
    {
        while (writing2List.load() || ls_.empty())
        {
            asm_pause();
        }
        writing2List.store(0, std::memory_order_acquire);
        while (!ls_.empty())
        {
            auto &line = ls_.front();

            char buffer[2048];
            memset(buffer, 0, sizeof(buffer));
            int len = line.stringify(buffer, sizeof(buffer) - 1);
            if (len != -1)
            {
                buffer[len++] = '\n';
                write(fd_.getFd(), buffer, len);

                struct stat st;
                fstat(fd_.getFd(), &st);
                if (st.st_size >= maxFileSize_ * 1048576)
                {
                    char loc[100];
                    memset(loc, 0, sizeof(loc));
                    sprintf(loc, "%s%s_%d", filePath_, fileName_, cnt++);
                    fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
                    assert(fd_.getFd() >= 0);
                }
            }

            ls_.pop_front();
        }
        writing2List.store(1, std::memory_order_release);
    }
}