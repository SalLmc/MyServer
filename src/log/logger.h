#ifndef LOGGER_H
#define LOGGER_H

#include "../headers.h"

extern bool enable_logger;

enum class Level
{
    INFO,
    WARN,
    CRIT
};

extern Level logger_level;

class LogLine
{
  public:
    LogLine();
    LogLine(Level level, char const *file, char const *function, unsigned int line);
    LogLine &operator<<(int8_t arg);
    LogLine &operator<<(uint8_t arg);
    LogLine &operator<<(int16_t arg);
    LogLine &operator<<(uint16_t arg);
    LogLine &operator<<(int32_t arg);
    LogLine &operator<<(uint32_t arg);
    LogLine &operator<<(int64_t arg);
    LogLine &operator<<(uint64_t arg);
    LogLine &operator<<(std::string &arg);
    LogLine &operator<<(std::string &&arg);
    LogLine &operator<<(const char *arg);

    char buffer_[1024];
    int pos_ = 0;

    Level level_;
};

class AtomicSpinlock
{
    std::atomic_flag mutex_;

  public:
    AtomicSpinlock() : mutex_(0)
    {
    }
    void lock()
    {
        while (mutex_.test_and_set())
            ;
    }
    void unlock()
    {
        mutex_.clear();
    }
    // @return 1 for success, 0 for failure
    bool tryLock()
    {
        return !mutex_.test_and_set();
    }

    AtomicSpinlock(const AtomicSpinlock &) = delete;
    AtomicSpinlock &operator=(const AtomicSpinlock &) = delete;
};

class Logger
{
  public:
    Logger() = delete;
    Logger(const char *rootPath, const char *logName);
    ~Logger();
    Logger &operator+=(LogLine &line);

    void write2FileInner();
    void write2File();

    void wakeup();
    void tryWakeup();

    const char *rootPath_;
    const char *logName_;

    unsigned long threshold_;

  private:
    enum class State
    {
        INIT,
        ACTIVE,
        SHUTDOWN
    };

    std::list<LogLine> ls_;

    int log_ = -1;

    std::atomic<State> state_;

    std::mutex mutex_;
    std::condition_variable cond_;

    std::thread writeThread_;
};

#define LOG_INFO                                                                                                       \
    if (enable_logger && logger_level <= Level::INFO)                                                                  \
    __LOG_INFO

#define LOG_WARN                                                                                                       \
    if (enable_logger && logger_level <= Level::WARN)                                                                  \
    __LOG_WARN

#define LOG_CRIT                                                                                                       \
    if (enable_logger && logger_level <= Level::CRIT)                                                                  \
    __LOG_CRIT

#define __LOG_INFO __LOG_INFO_INNER(*serverPtr->logger_)
#define __LOG_WARN __LOG_WARN_INNER(*serverPtr->logger_)
#define __LOG_CRIT __LOG_CRIT_INNER(*serverPtr->logger_)

#define __LOG_INFO_INNER(logger) (logger) += LogLine(Level::INFO, __FILE__, __func__, __LINE__)
#define __LOG_WARN_INNER(logger) (logger) += LogLine(Level::WARN, __FILE__, __func__, __LINE__)
#define __LOG_CRIT_INNER(logger) (logger) += LogLine(Level::CRIT, __FILE__, __func__, __LINE__)

#endif