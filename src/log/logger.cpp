#include "../headers.h"

#include "../utils/utils.h"
#include "logger.h"

LogLine::LogLine()
{
    memset(buffer_, 0, sizeof(buffer_));
    pos_ = 0;
}

LogLine::LogLine(Level level, char const *file, char const *function, unsigned int line) : level_(level)
{
    // level
    switch (level)
    {
    case Level::CRIT:
        snprintf(buffer_ + pos_, SIZE - pos_ - 1, "[CRIT] ");
        pos_ += 7;
        break;
    case Level::INFO:
        snprintf(buffer_ + pos_, SIZE - pos_ - 1, "[INFO] ");
        pos_ += 7;
        break;
    case Level::WARN:
        snprintf(buffer_ + pos_, SIZE - pos_ - 1, "[WARN] ");
        pos_ += 7;
        break;
    }

    // time
    auto timeStamp_ = getTickMs();
    auto shortTime = timeStamp_ / 1000;
    std::time_t time_t = shortTime;
    auto lctime = std::localtime(&time_t);
    strftime(buffer_ + pos_, 20, "[%Y-%m-%d %H:%M:%S", lctime);
    pos_ += 20;
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, ".%03lld] ", timeStamp_ % 1000);
    pos_ += 6;

    // pid
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "<%d> ", getpid());
    pos_ += strlen(buffer_ + pos_);

    // file
    strncpy(buffer_ + pos_, file, SIZE - pos_ - 1);
    pos_ += strlen(buffer_ + pos_);
    buffer_[pos_++] = ':';

    // line
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d ", line);
    pos_ += strlen(buffer_ + pos_);

    // function
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "{%s}", function);
    pos_ += strlen(buffer_ + pos_);
    buffer_[pos_++] = ' ';
}

LogLine &LogLine::operator<<(int8_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint8_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int16_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint16_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int32_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint32_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int64_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%ld", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint64_t arg)
{
    snprintf(buffer_ + pos_, SIZE - pos_ - 1, "%ld", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(std::string &arg)
{
    strncpy(buffer_ + pos_, arg.c_str(), SIZE - pos_ - 1);
    pos_ += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(std::string &&arg)
{
    strncpy(buffer_ + pos_, arg.c_str(), SIZE - pos_ - 1);
    pos_ += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(const char *arg)
{
    strncpy(buffer_ + pos_, arg, SIZE - pos_ - 1);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}

Logger::Logger(const char *rootPath, const char *logName)
    : rootPath_(rootPath), logName_(logName), threshold_(1), state_(State::INIT),
      writeThread_(&Logger::write2File, this)
{
    std::string file(rootPath);
    if (file.back() != '/')
    {
        file.append("/");
    }
    file.append(logName);

    if (access(rootPath, W_OK | R_OK | X_OK | F_OK) != 0)
    {
        recursiveMkdir(rootPath, 0777);
    }

    char log[100] = {0};
    snprintf(log, 99, "%s.log", file.c_str());

    log_ = open(log, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (log_ >= 0)
    {
        state_.store(State::ACTIVE);
    }
}
Logger::~Logger()
{
    state_.store(State::SHUTDOWN);

    {
        std::unique_lock<std::mutex> ulock(mutex_);
        cond_.notify_all();
    }

    writeThread_.join();
    if (log_ != -1)
    {
        close(log_);
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
        line.buffer_[line.pos_++] = '\n';

        write(log_, line.buffer_, line.pos_);

        ls_.pop_front();
    }
}