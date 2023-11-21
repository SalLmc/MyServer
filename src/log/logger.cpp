#include "../headers.h"

#include "../utils/utils_declaration.h"
#include "logger.h"

bool enable_logger = 1;

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
        sprintf(buffer_ + pos_, "[CRIT] ");
        pos_ += 7;
        break;
    case Level::INFO:
        sprintf(buffer_ + pos_, "[INFO] ");
        pos_ += 7;
        break;
    case Level::WARN:
        sprintf(buffer_ + pos_, "[WARN] ");
        pos_ += 7;
        break;
    }

    // time
    auto timeStamp_ = getTickMs();
    auto shortTime = timeStamp_ / 1000;
    std::time_t time_t = shortTime;
    auto lctime = std::localtime(&time_t);
    strftime(buffer_ + pos_, 21, "[%Y-%m-%d %H:%M:%S", lctime);
    pos_ += 20;
    sprintf(buffer_ + pos_, ".%03lld] ", timeStamp_ % 1000);
    pos_ += 6;

    // pid
    sprintf(buffer_ + pos_, "<%d> ", getpid());
    pos_ += strlen(buffer_ + pos_);

    // file
    strcpy(buffer_ + pos_, file);
    pos_ += strlen(buffer_ + pos_);
    buffer_[pos_++] = ':';

    // line
    sprintf(buffer_ + pos_, "%d ", line);
    pos_ += strlen(buffer_ + pos_);

    // function
    sprintf(buffer_ + pos_, "{%s}", function);
    pos_ += strlen(buffer_ + pos_);
    buffer_[pos_++] = ' ';
}

LogLine &LogLine::operator<<(int8_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint8_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int16_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint16_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int32_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint32_t arg)
{
    sprintf(buffer_ + pos_, "%d", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(int64_t arg)
{
    sprintf(buffer_ + pos_, "%ld", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(uint64_t arg)
{
    sprintf(buffer_ + pos_, "%ld", arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}
LogLine &LogLine::operator<<(std::string &arg)
{
    strcpy(buffer_ + pos_, arg.c_str());
    pos_ += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(std::string &&arg)
{
    strcpy(buffer_ + pos_, arg.c_str());
    pos_ += arg.size();
    return *this;
}
LogLine &LogLine::operator<<(const char *arg)
{
    strcpy(buffer_ + pos_, arg);
    pos_ += strlen(buffer_ + pos_);
    return *this;
}

Logger::Logger(const char *rootPath, const char *logName)
    : rootPath_(rootPath), logName_(logName), threshold_(1), state_(State::INIT),
      writeThread_(&Logger::write2File, this)
{
    std::string folder(rootPath);
    if (folder.back() != '/')
    {
        folder.append("/");
    }
    folder.append(logName);

    if (access(folder.data(), W_OK | R_OK | X_OK | F_OK) != 0)
    {
        recursiveMkdir(folder.data(), 0777);
    }

    char info[100] = {0};
    sprintf(info, "%s/info.log", folder.c_str());
    char warn[100] = {0};
    sprintf(warn, "%s/warn.log", folder.c_str());
    char error[100] = {0};
    sprintf(error, "%s/error.log", folder.c_str());

    info_ = open(info, O_RDWR | O_CREAT | O_TRUNC, 0666);
    warn_ = open(warn, O_RDWR | O_CREAT | O_TRUNC, 0666);
    error_ = open(error, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (info_ >= 0 && warn_ >= 0 && error_ >= 0)
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
        line.buffer_[line.pos_++] = '\n';

        switch (line.level_)
        {
        case Level::INFO:
            write(info_, line.buffer_, line.pos_);
            break;
        case Level::WARN:
            write(warn_, line.buffer_, line.pos_);
            break;
        case Level::CRIT:
            write(error_, line.buffer_, line.pos_);
            break;
        default:
            break;
        }

        ls_.pop_front();
    }
}