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

class LogLine
{
  public:
    LogLine();
    LogLine(Level level, char const *file, char const *function, unsigned int line);
    int stringify(char *buffer, int n);
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
    int pos = 0;

    Level level;
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
    Logger(const char *path, const char *name);
    ~Logger();
    Logger &operator+=(LogLine &line);

    void write2FileInner();
    void write2File();

    void wakeup();

    const char *filePath_;
    const char *fileName_;

  private:
    enum class State
    {
        INIT,
        ACTIVE,
        SHUTDOWN
    };
    std::list<LogLine> ls_;

    int info_ = -1;
    int warn_ = -1;
    int error_ = -1;

    std::atomic<State> state_;

    std::mutex mutex_;
    std::condition_variable cond_;

    std::thread writeThread_;
};

#define LOG_INFO                                                                                                       \
    if (enable_logger)                                                                                                 \
    __LOG_INFO

#define LOG_WARN                                                                                                       \
    if (enable_logger)                                                                                                 \
    __LOG_WARN

#define LOG_CRIT                                                                                                       \
    if (enable_logger)                                                                                                 \
    __LOG_CRIT

#define __LOG_INFO __LOG_INFO_INNER(*cyclePtr->logger_)
#define __LOG_WARN __LOG_WARN_INNER(*cyclePtr->logger_)
#define __LOG_CRIT __LOG_CRIT_INNER(*cyclePtr->logger_)

#define __LOG_INFO_INNER(logger) (logger) += LogLine(Level::INFO, __FILE__, __func__, __LINE__)
#define __LOG_WARN_INNER(logger) (logger) += LogLine(Level::WARN, __FILE__, __func__, __LINE__)
#define __LOG_CRIT_INNER(logger) (logger) += LogLine(Level::CRIT, __FILE__, __func__, __LINE__)

#endif