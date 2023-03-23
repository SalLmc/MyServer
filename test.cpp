#include <arpa/inet.h>
#include <assert.h>
#include <atomic>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "src/core/core.h"

int main()
{
    ServerAttribute a(8081, "/home/sallmc/dist", "index.html", "/api/", "http://175.178.175.106:8080/", 0,{"index.html"});

    for(auto &x:a.try_files)
    {
        printf("%s\n",x.c_str());
    }
}