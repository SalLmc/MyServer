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

int pt(void *arg)
{
    printf("%s\n", (char *)arg);
}

int main()
{
    HeapTimer timer;
    char *arg = "hello";

    TimerNode node;
    auto now = getTickMs();
    printf("now:%lld\n", now);
    timer.Add(1, now + 1000, pt, (void *)arg);
    timer.Add(2, now + 2000, pt, (void *)arg);

    printf("next tick:%lld\n", timer.GetNextTick());

    while (getTickMs() < now + 2000)
    {
        timer.Tick();
    }

}