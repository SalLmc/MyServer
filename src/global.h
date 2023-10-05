#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_PROCESS_N 16

// #define LOOP_ACCEPT

// config

extern bool enable_logger;
extern unsigned long logger_wake;
extern bool is_daemon;
extern bool only_worker;
extern bool use_epoll;
extern int processes;

// runtime

extern long cores;
extern bool isChild;
extern bool quit;
extern bool restart;
extern int slot;
extern bool useAcceptMutex;
extern bool acceptMutexHeld;

#endif