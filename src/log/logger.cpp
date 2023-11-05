#include "../headers.h"

#include "../utils/utils_declaration.h"
#include "logger.h"

bool enable_logger = 1;

LogLine::LogLine()
{
    memset(buffer_, 0, sizeof(buffer_));
    pos = 0;
}

LogLine::LogLine(Level level, char const *file, char const *function, unsigned int line) : level(level)
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
    auto lctime = std::localtime(&time_t);
    strftime(buffer_ + pos, 21, "[%Y-%m-%d %H:%M:%S", lctime);
    pos += 20;
    sprintf(buffer_ + pos, ".%03lld] ", timeStamp_ % 1000);
    pos += 6;

    // pid
    sprintf(buffer_ + pos, "<%d> ", getpid());
    pos += strlen(buffer_ + pos);

    // file
    strcpy(buffer_ + pos, file);
    pos += strlen(buffer_ + pos);
    buffer_[pos++] = ':';

    // line
    sprintf(buffer_ + pos, "%d ", line);
    pos += strlen(buffer_ + pos);

    // function
    sprintf(buffer_ + pos, "{%s}", function);
    pos += strlen(buffer_ + pos);
    buffer_[pos++] = ' ';
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

Logger::Logger(const char *path, const char *name)
    : filePath_(path), fileName_(name), threshold_(1), state_(State::INIT), writeThread_(&Logger::write2File, this)
{
    std::string folder(filePath_);
    folder.append(fileName_);

    if (access(folder.data(), W_OK | R_OK | X_OK | F_OK) != 0)
    {
        recursiveMkdir(folder.data(), 0777);
    }

    char info[100] = {0};
    sprintf(info, "%s%s/info.log", filePath_, fileName_);
    char warn[100] = {0};
    sprintf(warn, "%s%s/warn.log", filePath_, fileName_);
    char error[100] = {0};
    sprintf(error, "%s%s/error.log", filePath_, fileName_);

    info_ = open(info, O_RDWR | O_CREAT | O_TRUNC, 0666);
    warn_ = open(warn, O_RDWR | O_CREAT | O_TRUNC, 0666);
    error_ = open(error, O_RDWR | O_CREAT | O_TRUNC, 0666);

    assert(info_ >= 0 && warn_ >= 0 && error_ >= 0);
    state_.store(State::ACTIVE);
}
Logger::~Logger()
{
    state_.store(State::SHUTDOWN);

    {
        std::unique_lock<std::mutex> ulock(mutex_);
        cond_.notify_all();
    }

    writeThread_.join();
    if (info_ != -1)
    {
        close(info_);
    }
    if (warn_ != -1)
    {
        close(warn_);
    }
    if (error_ != -1)
    {
        close(error_);
    }
}
void Logger::wakeup()
{
    if (!ls_.empty())
    {
        cond_.notify_one();
    }
}
void Logger::tryWakeup()
{
    if (ls_.size() >= threshold_)
    {
        wakeup();
    }
}
Logger &Logger::operator+=(LogLine &line)
{
    std::unique_lock<std::mutex> ulock(mutex_);
    ls_.push_back(std::move(line));

    tryWakeup();

    return *this;
}

void Logger::write2File()
{
    while (state_.load() == State::INIT)
    {
        asm_pause();
    }

    while (state_.load() == State::ACTIVE)
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        while (ls_.empty() && state_.load() == State::ACTIVE)
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
        line.buffer_[line.pos++] = '\n';

        switch (line.level)
        {
        case Level::INFO:
            write(info_, line.buffer_, line.pos);
            break;
        case Level::WARN:
            write(warn_, line.buffer_, line.pos);
            break;
        case Level::CRIT:
            write(error_, line.buffer_, line.pos);
            break;
        default:
            break;
        }

        ls_.pop_front();
    }
}