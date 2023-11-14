#ifndef PROCESS_H
#define PROCESS_H

#include "../headers.h"

class Server;
class Event;

void masterProcessCycle(Server *server);
void workerProcessCycle(Server *server);
void startWorkerProcesses(Server *server, int n);
pid_t spawnProcesses(Server *server, std::function<void(Server *)> proc);
int recvFromMaster(Event *rev);
void signalWorkerProcesses(int sig);
void processEventsAndTimers(Server *server);

int logging(void *arg);

#endif