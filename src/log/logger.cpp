#include "../headers.h"

#include "logger.h"
#include "../util/utils_declaration.h"

LogLine::LogLine()
{
    memset(buffer_, 0, sizeof(buffer_));
    pos = 0;
}

LogLine::LogLine(Level level, char const *file, char const *function, unsigned int line)
{
    // level
    switch (level)
    {
    case Level::CRIT:
        sprintf(buffer_ + pos, "[CRIT] ");
        pos += 7;
        break;
    case Level::INFO:
        sprintf(buffer_ + pos, "[INFO] ");
        pos += 7;
        break;
    case Level::WARN:
        sprintf(buffer_ + pos, "[WARN] ");
        pos += 7;
        break;
    }

    // time
    auto timeStamp_ = getTickMs();
    auto shortTime = timeStamp_ / 1000;
    std::time_t time_t = shortTime;
    auto gmtime = std::gmtime(&time_t);
    gmtime->tm_hour += 8;
    strftime(buffer_ + pos, 23, "[%Y-%m-%d %H:%M:%S] ", gmtime);
    pos += 22;

    // pid
    sprintf(buffer_ + pos, "<%d> ", getpid());
    pos += strlen(buffer_ + pos);

    // file
    strcpy(buffer_ + pos, file);
    pos += strlen(buffer_ + pos);
    buffer_[pos++] = ' ';

    // function
    strcpy(buffer_ + pos, function);
    pos += strlen(buffer_ + pos);
    buffer_[pos++] = ':';

    // line
    sprintf(buffer_ + pos, "%d ", line);
    pos += strlen(buffer_ + pos);
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
    pos += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(std::string &&arg)
{
    strcpy(buffer_ + pos, arg.c_str());
    pos += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(const char *arg)
{
    strcpy(buffer_ + pos, arg);
    pos += strlen(buffer_ + pos);
    return *this;
}

Logger::Logger(const char *path, const char *name, unsigned int size_mb)
    : filePath_(path), fileName_(name), maxFileSize_(size_mb), cnt(1), state(State::INIT),
      writeThread(&Logger::write2File, this)
{
    if (access(path, 0) != 0)
    {
        mkdir(path, 0744);
    }

    char loc[100];
    memset(loc, 0, sizeof(loc));
    sprintf(loc, "%s%s_%d.txt", filePath_, fileName_, cnt++);
    fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
    assert(fd_ >= 0);
    state.store(State::ACTIVE);
}
Logger::~Logger()
{
    state.store(State::SHUTDOWN);
#ifndef USE_ATOMIC_LOCK
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        cond_.notify_all();
    }
#endif
    writeThread.join();
    if (fd_ != -1)
    {
        close(fd_);
    }
}
Logger &Logger::operator+=(LogLine &line)
{
#ifndef USE_ATOMIC_LOCK
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        ls_.push_back(std::move(line));
    }
    cond_.notify_one();
#else
    spLock.lock();
    ls_.push_back(std::move(line));
    spLock.unlock();
#endif
    return *this;
}

void Logger::write2File()
{
    while (state.load() == State::INIT)
    {
        asm_pause();
    }

    while (state.load() == State::ACTIVE)
    {
#ifndef USE_ATOMIC_LOCK
        std::unique_lock<std::mutex> ulock(mutex_);
        while (ls_.empty() && state.load() == State::ACTIVE)
        {
            cond_.wait(ulock);
        }
        write2FileInner();
#else
        if (spLock.tryLock())
        {
            if (!ls_.empty())
            {
                write2FileInner();
            }
            spLock.unlock();
        }
        else
        {
            asm_pause();
        }
#endif
    }

    write2FileInner();
}

void Logger::write2FileInner()
{
    while (!ls_.empty())
    {
        auto &line = ls_.front();
        line.buffer_[line.pos++] = '\n';

        write(fd_, line.buffer_, line.pos);

        struct stat st;
        int ret = fstat(fd_, &st);
        if (ret == 0 && st.st_size >= maxFileSize_ * 1048576)
        {
            char loc[100];
            memset(loc, 0, sizeof(loc));
            sprintf(loc, "%s%s_%d.txt", filePath_, fileName_, cnt++);
            fd_ = open(loc, O_RDWR | O_CREAT | O_TRUNC, 0666);
            assert(fd_ >= 0);
        }
        ls_.pop_front();
    }
}