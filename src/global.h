#ifndef GLOBAL_H
#define GLOBAL_H

#define MAX_PROCESS_N 16

// #define USE_ATOMIC_LOCK
// #define RE_ALLOC

extern bool isChild;
extern bool quit;
extern bool restart;
extern int slot;
extern bool useAcceptMutex;
extern bool acceptMutexHeld;

#endif