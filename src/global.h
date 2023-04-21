#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_PROCESS_N 16

#define PROCESS 8

// #define RE_ALLOC
// #define LOOP_ACCEPT
#define ENABLE_LOGGER
// #define LOGGER_IS_SYNC

extern bool isChild;
extern bool quit;
extern bool restart;
extern int slot;
extern bool useAcceptMutex;
extern bool acceptMutexHeld;

#endif