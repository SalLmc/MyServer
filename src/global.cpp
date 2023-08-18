#include "global.h"

int process_n = 2;
unsigned long logger_wake = 100;
int only_worker = 0;

bool isChild;
bool quit = 0;
bool restart = 0;
int slot = 0;
bool useAcceptMutex = 0;
bool acceptMutexHeld = 0;
long cores;