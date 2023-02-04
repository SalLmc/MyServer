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

class node
{
  public:
    int x = 1;
    node()
    {
        printf("constructing\n");
    }
    node(int n)
    {
        x = n;
        printf("%d is constructing\n", x);
    }
    ~node()
    {
        printf("%d is destructing\n", x);
    }
};

int main(int argc, char *argv[])
{
    std::unique_ptr<Logger> lg(new Logger("log/", "startup", 10));
    LOG_CRIT(*lg) << "start";
    lg.reset();
    lg.reset(new Logger("log/", "anotherstartup", 10));
    LOG_CRIT(*lg) << "anotherstart";

    switch (fork())
    {
    case 0:
        printf("childpid:%d\n", getpid());
        lg.reset();
        lg.reset(new Logger("log/", "child", 10));
        LOG_INFO(*lg) << "child";
        break;

    default:
        printf("fatherpid:%d\n", getpid());
        lg.reset();
        lg.reset(new Logger("log/", "father", 10));
        LOG_INFO(*lg) << "father";
        break;
    }
}