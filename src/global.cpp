#include "global.h"

int process_n = 1;
unsigned long logger_wake = 1;
int only_worker = 0;
int enable_logger = 1;

bool isChild;
bool quit = 0;
bool restart = 0;
int slot = 0;
bool useAcceptMutex = 0;
bool acceptMutexHeld = 0;
long cores;