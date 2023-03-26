#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "../global.h"

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
    Logger(const char *path, const char *name, unsigned int size_mb);
    ~Logger();
    Logger &operator+=(LogLine &line);

    void write2FileInner();
    void write2File();

    const char *filePath_;
    const char *fileName_;
    unsigned int maxFileSize_;

  private:
    enum class State
    {
        INIT,
        ACTIVE,
        SHUTDOWN
    };
    std::list<LogLine> ls_;

    int fd_ = -1;
    int cnt;

    std::atomic<State> state;

#ifndef USE_ATOMIC_LOCK
    std::mutex mutex_;
    std::condition_variable cond_;
#else
    AtomicSpinlock spLock;
#endif

    std::thread writeThread;
};

#define LOG_INFO (*cyclePtr->logger_) += LogLine(Level::INFO, __FILE__, __func__, __LINE__)
#define LOG_WARN (*cyclePtr->logger_) += LogLine(Level::WARN, __FILE__, __func__, __LINE__)
#define LOG_CRIT (*cyclePtr->logger_) += LogLine(Level::CRIT, __FILE__, __func__, __LINE__)

#define LOG_INFO_BY(logger) (logger) += LogLine(Level::INFO, __FILE__, __func__, __LINE__)
#define LOG_WARN_BY(logger) (logger) += LogLine(Level::WARN, __FILE__, __func__, __LINE__)
#define LOG_CRIT_BY(logger) (logger) += LogLine(Level::CRIT, __FILE__, __func__, __LINE__)

#endif