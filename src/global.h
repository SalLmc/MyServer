#ifndef GLOBAL_H
#define GLOBAL_H

#include "core/core.h"
#include "event/epoller.h"
#include "util/utils_declaration.h"
#include "vector"

struct SignalWrapper;

extern ConnectionPool pool;
extern Epoller epoller;
extern SignalWrapper signals[];
extern bool quit;
extern bool restart;

#endif