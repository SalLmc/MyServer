#ifndef GLOBAL_H
#define GLOBAL_H

#include "core/core.h"
#include "event/epoller.h"
#include "util/utils_declaration.h"
#include <unordered_map>
#include <vector>
#include <list>

#define MAX_PROCESS_N 16

struct SignalWrapper;

extern ConnectionPool pool;
extern Epoller epoller;

extern SignalWrapper signals[];
extern bool quit;
extern bool restart;

extern std::unordered_map<std::string, std::string> mp;

extern Process processes[MAX_PROCESS_N];
extern int slot;

#endif