#include "global.h"

// config

Config serverConfig = {
    .loggerEnable = 1,
    .loggerThreshold = 1,
    .loggerInterval = 3,
    .daemon = 0,
    .onlyWorker = 0,
    .processes = 2,
    .useEpoll = 1,
    .connections = 1024,
    .eventDelay = 1,
};

// runtime

bool isChild;
bool quit = 0;
int slot = 0;