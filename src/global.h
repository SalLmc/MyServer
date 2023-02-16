#ifndef GLOBAL_H
#define GLOBAL_H

#include <list>
#include <unordered_map>
#include <vector>

#define MAX_PROCESS_N 16

class Cycle;
class ConnectionPool;
class RequestPool;
class Epoller;
struct SignalWrapper;
class Process;
class sharedMemory;
class ProcessMutex;
class HeapMemory;

extern bool isChild;

extern Cycle *cyclePtr;

extern ConnectionPool cPool;
extern RequestPool rPool;
extern Epoller epoller;

extern SignalWrapper signals[];
extern bool quit;
extern bool restart;

extern std::unordered_map<std::string, std::string> mp;

extern Process processes[MAX_PROCESS_N];
extern int slot;

extern bool useAcceptMutex;
extern sharedMemory shmForAMtx;
extern bool acceptMutexHeld;
extern ProcessMutex acceptMutex;

extern HeapMemory heap;

#endif