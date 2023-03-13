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
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include "logger.h"

Logger logger("log/","test",1);

int main(int argc, char *argv[])
{
    LOG_INFO_BY(logger)<<"TEST";
}