#include "global.h"

#include "core/core.h"
#include "event/epoller.h"
#include "event/event.h"
#include "http/http.h"
#include "util/utils_declaration.h"

#include "core/memory_manage.hpp"

bool isChild;

Cycle *cyclePtr = NULL;

ConnectionPool cPool;
RequestPool rPool;
Epoller epoller;

SignalWrapper signals[] = {{SIGINT, signal_handler, "stop"}, {SIGUSR1, signal_handler, "reload"}, {0, NULL, ""}};
bool quit = 0;
bool restart = 0;

std::unordered_map<std::string, std::string> mp;

Process processes[MAX_PROCESS_N];
int slot = 0;

bool useAcceptMutex = 1;
sharedMemory shmForAMtx;
bool acceptMutexHeld = 0;
ProcessMutex acceptMutex;

HeapMemory heap;