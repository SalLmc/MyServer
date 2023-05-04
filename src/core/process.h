#ifndef PROCESS_H
#define PROCESS_H

#include "../headers.h"

class Cycle;
class Event;

void masterProcessCycle(Cycle *cycle);
void workerProcessCycle(Cycle *cycle);
void startWorkerProcesses(Cycle *cycle, int n);
pid_t spawnProcesses(Cycle *cycle, std::function<void(Cycle *)> proc);
int recvFromMaster(Event *rev);
void signalWorkerProcesses(int sig);
void processEventsAndTimers(Cycle *cycle);

int recoverRequests(void *arg);

#endif