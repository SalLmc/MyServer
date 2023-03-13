#include "src/core/core.h"
#include "src/global.h"
#include "src/util/utils_declaration.h"
#include <bits/stdc++.h>
#include <stdio.h>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>
#include <vector>

#include "src/log/logger.h"

extern ConnectionPool cPool;;

int main(int argc, char *argv[])
{

    Cycle cycle(&cPool, new Logger("log/", "startup", 10));

    for (int i = 0; i < 10; i++)
        LOG_CRIT_BY(*cycle.logger_) << "start";
    printf("STARTUP END\n");

    delete cycle.logger_;

    switch (fork())
    {
    case 0:
        cycle.logger_ = new Logger("log/", "child", 10);
        for (int i = 0; i < 10; i++)
            LOG_INFO_BY(*cycle.logger_) << "child";
        printf("CHILD END\n");
        break;

    default:
        cycle.logger_ = new Logger("log/", "father", 10);
        for (int i = 0; i < 10; i++)
            LOG_INFO_BY(*cycle.logger_) << "father";
        printf("FATHER END\n");
        break;
    }
    // exit(0);
}