#include "global.h"

ConnectionPool pool;
Epoller epoller;

SignalWrapper signals[] = {{SIGINT, signal_handler, "stop"}, {SIGUSR1, signal_handler, "reload"}, {0, NULL, ""}};
bool quit = 0;
bool restart = 0;

std::unordered_map<std::string, std::string> mp;

Process processes[MAX_PROCESS_N];
int slot=0;