#include "global.h"

int process_n = 1;
unsigned long logger_wake = 1;
bool only_worker = 0;
bool enable_logger = 1;
bool use_epoll = 1;
bool is_daemon = 0;
bool isChild;
bool quit = 0;
bool restart = 0;
int slot = 0;
bool useAcceptMutex = 0;
bool acceptMutexHeld = 0;
long cores;