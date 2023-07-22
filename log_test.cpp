#include "src/headers.h"

#include "src/core/core.h"
#include "src/global.h"
#include "src/log/logger.h"
#include "src/util/utils_declaration.h"

extern ConnectionPool cPool;

int main(int argc, char *argv[])
{

    Cycle cycle(&cPool, new Logger("log/", "startup", 10));

    for (int i = 0; i < 10; i++)
        __LOG_CRIT_INNER(*cycle.logger_) << "start";
    printf("STARTUP END\n");

    delete cycle.logger_;

    switch (fork())
    {
    case 0:
        cycle.logger_ = new Logger("log/", "child", 10);
        for (int i = 0; i < 10; i++)
            __LOG_CRIT_INNER(*cycle.logger_) << "child";
        printf("CHILD END\n");
        break;

    default:
        cycle.logger_ = new Logger("log/", "father", 10);
        for (int i = 0; i < 10; i++)
            __LOG_CRIT_INNER(*cycle.logger_) << "father";
        printf("FATHER END\n");
        break;
    }
    // exit(0);
}