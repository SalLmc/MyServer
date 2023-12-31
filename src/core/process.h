#ifndef PROCESS_H
#define PROCESS_H

#include "../headers.h"

class Server;
class Event;

void master(Server *server);
void worker(Server *server);
void startWorkerProcesses(Server *server, int n);
pid_t spawnProcesses(Server *server, std::function<void(Server *)> proc);
void signalWorkerProcesses(int sig);

int logging(void *arg);

#endif