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

int main(int argc, char *argv[])
{
    // std::unique_ptr<Logger> lg(new Logger("log/", "startup", 10));
    Logger *lg = new Logger("log/", "startup", 10);

    for (int i = 0; i < 10; i++)
        LOG_CRIT(*lg) << "start";
    printf("STARTUP END\n");

    delete lg;

    switch (fork())
    {
    case 0:
        lg = new Logger("log/", "child", 10);
        // lg.reset(new Logger("log/", "child", 10));
        for (int i = 0; i < 10; i++)
            LOG_INFO(*lg) << "child";
        printf("CHILD END\n");
        break;

    default:
        lg = new Logger("log/", "father", 10);
        // lg.reset(new Logger("log/", "father", 10));
        for (int i = 0; i < 10; i++)
            LOG_INFO(*lg) << "father";
        printf("FATHER END\n");
        break;
    }

    delete lg;
}