#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/event/event.h"
#include "../src/log/logger.h"
#include "../src/timer/timer.h"
#include "../src/utils/utils.h"

Level loggerLevel = Level::INFO;
class Server;
bool enableLogger;
Server *serverPtr;

int main()
{
    HeapTimer timer;
    printf("time:%lld\n", getTickMs());
    timer.add(
        1, "test", getTickMs(),
        [&](void *arg) {
            printf("timer msg:%s\n", (char *)arg);
            printf("time triggered:%lld\n", getTickMs());
            timer.again(1, getTickMs() + 1000);
            return 1;
        },
        (void *)"HELLO");
    timer.printActiveTimer();
    timer.tick();
    sleep(1);
    timer.tick();
}