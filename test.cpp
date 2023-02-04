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

Logger lg("log/", "test", 10);

int main(int argc, char *argv[])
{
    for (int i = 0; i < 10; i++)
    {
        LOG_INFO(lg) << "hello test " << 1;
    }
}