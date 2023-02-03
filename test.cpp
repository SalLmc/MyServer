#include "src/core/core.h"
#include "src/global.h"
#include "src/log/nanolog.hpp"
#include "src/util/utils_declaration.h"
#include <bits/stdc++.h>
#include <stdio.h>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>
#include <vector>
#define POOLSIZE 32

int main(int argc, char *argv[])
{
    nanolog::initialize(nanolog::GuaranteedLogger(), "log/", "log", 1);
    int fd;
    assert(fd=open("pid_file",O_RDWR)>=0);
    close(fd);
    close(fd);
}