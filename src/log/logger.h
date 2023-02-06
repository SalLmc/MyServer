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
    LogLine &operator<<(const char *arg);

    char const *file_;
    char const *function_;

    unsigned int line_;
    char cLine_[16];

    Level level_;
    char const *cLevel_;

    char ctimeStamp_[32];
    unsigned long long timeStamp_;

    char cPid_[16];
    pid_t pid_;

    char buffer_[1024];
    int pos = 0;
};

class Logger
{
  public:
    Logger() = delete;
    Logger(const char *path, const char *name, unsigned long long size);
    ~Logger();
    Logger &operator+=(LogLine &line);

    void write2FileInner();
    void write2File();

    const char *filePath_;
    const char *fileName_;
    unsigned long long maxFileSize_;

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

    std::mutex mutex_;
    std::condition_variable cond_;

    std::thread writeThread;
};

#define LOG_INFO (*cyclePtr->logger_) += LogLine(Level::INFO, __FILE__, __func__, __LINE__)
#define LOG_WARN (*cyclePtr->logger_) += LogLine(Level::WARN, __FILE__, __func__, __LINE__)
#define LOG_CRIT (*cyclePtr->logger_) += LogLine(Level::CRIT, __FILE__, __func__, __LINE__)

#endif