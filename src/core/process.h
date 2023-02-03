#ifndef PROCESS_H
#define PROCESS_H

#include "core.h"
#include <functional>

void masterProcessCycle(Cycle *cycle);
void workerProcessCycle(Cycle *cycle);

void startWorkerProcesses(Cycle *cycle, int n);

pid_t spawnProcesses(Cycle *cycle, std::function<void(Cycle *)> proc);

int recvFromMaster(Event *rev);
int recvFromWorker(Event *rev);

void signalWorkerProcesses(int sig);

#endif