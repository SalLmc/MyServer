#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_PROCESS_N 16

// #define LOOP_ACCEPT
// #define LOG_HEADER

// config

struct Config
{
    bool loggerEnable;
    unsigned long loggerThreshold;
    int loggerInterval;

    bool daemon;
    bool onlyWorker;
    int processes;

    bool useEpoll;
    int connections;
    int eventDelay;
};

extern Config serverConfig;

// runtime

extern bool isChild;
extern bool quit;
extern int slot;

#endif