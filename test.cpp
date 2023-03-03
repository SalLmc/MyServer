#include "src/http/http.h"
#include "src/util/utils_declaration.h"
#include <arpa/inet.h>
#include <assert.h>
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

int main(int argc, char *argv[])
{
    std::string addr = "http://175.178.175.106/api";

    printf("%s\n",getNewUri(addr).c_str());

}
