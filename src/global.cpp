#include "global.h"

bool isChild;

Cycle *cyclePtr = NULL;

ConnectionPool pool;
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