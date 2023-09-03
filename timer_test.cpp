#include "src/headers.h"

#include "src/core/core.h"
#include "src/event/event.h"
#include "src/timer/timer.h"
#include "src/utils/utils_declaration.h"

int main()
{
    HeapTimer timer;
    printf("time:%lld\n", getTickMs());
    timer.Add(
        1, getTickMs(),
        [&](void *arg) {
            printf("timer msg:%s\n", (char *)arg);
            printf("time triggered:%lld\n", getTickMs());
            timer.Again(1, getTickMs() + 1000);
            return 1;
        },
        (void *)"HELLO");
    timer.Tick();
    timer.Tick();
    timer.Tick();
}