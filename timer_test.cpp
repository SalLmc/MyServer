#include "src/core/core.h"
#include "src/event/event.h"
#include "src/timer/timer.h"
#include "src/util/utils_declaration.h"
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    HeapTimer timer;
    printf("time:%lld\n",getTickMs());
    timer.Add(
        1, getTickMs(),
        [&](void *arg) {
            printf("timer msg:%s\n", (char *)arg);
            printf("time triggered:%lld\n",getTickMs());
            timer.Again(1, getTickMs() + 1000);
            return 1;
        },
        (void *)"HELLO");
    timer.Tick();
    timer.Tick();
    timer.Tick();
}