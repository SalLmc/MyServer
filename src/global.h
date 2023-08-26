#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_PROCESS_N 16

// #define RE_ALLOC
// #define LOOP_ACCEPT

extern int process_n;
extern unsigned long logger_wake;
extern int only_worker;
extern int enable_logger;
extern bool isChild;
extern bool quit;
extern bool restart;
extern int slot;
extern bool useAcceptMutex;
extern bool acceptMutexHeld;
extern long cores;

#endif