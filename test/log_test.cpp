#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/global.h"
#include "../src/log/logger.h"
#include "../src/utils/utils_declaration.h"

extern ConnectionPool cPool;

int main(int argc, char *argv[])
{

    Server server(&cPool, new Logger("log/", "startup"));

    for (int i = 0; i < 10; i++)
        __LOG_CRIT_INNER(*server.logger_) << "start";
    printf("STARTUP END\n");

    delete server.logger_;

    switch (fork())
    {
    case 0:
        server.logger_ = new Logger("log/", "child");
        for (int i = 0; i < 10; i++)
            __LOG_CRIT_INNER(*server.logger_) << "child";
        printf("CHILD END\n");
        break;

    default:
        server.logger_ = new Logger("log/", "father");
        for (int i = 0; i < 10; i++)
            __LOG_CRIT_INNER(*server.logger_) << "father";
        printf("FATHER END\n");
        break;
    }
    // exit(0);
}