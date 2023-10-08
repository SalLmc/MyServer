#include "../src/headers.h"
#include "../src/log/logger.h"
#include "../src/utils/utils_declaration.h"

using namespace std;

int main()
{
    Logger logger("log/", "test");
    string ip = "127.0.0.1";
    int port = 8080;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr_;
    addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    addr_.sin_port = htons(port);
    if (connect(fd, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
    {
        __LOG_INFO_INNER(logger) << "errno:" << strerror(errno);
    }
    else
    {
        printf("OK\n");
    }
}