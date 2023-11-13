#ifndef PROCESS_H
#define PROCESS_H

#include "../headers.h"

class Server;
class Event;

void masterProcessCycle(Server *cycle);
void workerProcessCycle(Server *cycle);
void startWorkerProcesses(Server *cycle, int n);
pid_t spawnProcesses(Server *cycle, std::function<void(Server *)> proc);
int recvFromMaster(Event *rev);
void signalWorkerProcesses(int sig);
void processEventsAndTimers(Server *cycle);

int logging(void *arg);

#endif