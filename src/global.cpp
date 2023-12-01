#include "global.h"

// config

unsigned long logger_threshold = 1;
bool is_daemon = 0;
bool only_worker = 0;
bool use_epoll = 1;
int processes = 1;
int delay = 1;
int connections = 1024;

// runtime

int cores;
bool isChild;
bool quit = 0;
int slot = 0;
bool useAcceptMutex = 0;
bool acceptMutexHeld = 0;
