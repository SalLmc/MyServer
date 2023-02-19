#include "logger.h"
#include "../util/utils_declaration.h"
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

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
LogLine &LogLine::operator<<(std::string arg)
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
    : filePath_(path), fileName_(name), maxFileSize_(size), cnt(1), state(State::INIT),
      writeThread(&Logger::write2File, this)
{
    char loc[100];
    memset(loc, 0, sizeof(loc));
    sprintf(loc, "%s%s_%d.txt", filePath_, fileName_, cnt++);
    fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
    assert(fd_ >= 0);
    state.store(State::ACTIVE, std::memory_order_release);
}
Logger::~Logger()
{
    state.store(State::SHUTDOWN);
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        cond_.notify_all();
    }
    writeThread.join();
    if (fd_ != -1)
    {
        close(fd_);
    }
}
Logger &Logger::operator+=(LogLine &line)
{
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        ls_.push_back(line);
    }
    cond_.notify_one();
    return *this;
}

void Logger::write2File()
{
    while (state.load(std::memory_order_acquire) == State::INIT)
    {
    }

    while (state.load() == State::ACTIVE)
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        while (ls_.empty() && state.load() == State::ACTIVE)
        {
            cond_.wait(ulock);
        }
        write2FileInner();
    }

    write2FileInner();
}

void Logger::write2FileInner()
{
    while (!ls_.empty())
    {
        auto &line = ls_.front();

        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        int len = line.stringify(buffer, sizeof(buffer) - 1);
        if (len != -1)
        {
            buffer[len++] = '\n';
            write(fd_, buffer, len);

            struct stat st;
            fstat(fd_, &st);
            if (st.st_size >= maxFileSize_ * 1048576)
            {
                char loc[100];
                memset(loc, 0, sizeof(loc));
                sprintf(loc, "%s%s_%d.txt", filePath_, fileName_, cnt++);
                fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
                assert(fd_ >= 0);
            }
        }
        ls_.pop_front();
    }
}