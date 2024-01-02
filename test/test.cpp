#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/utils/utils.h"

Server *serverPtr;
bool enable_logger = 1;

int main()
{
    int val = 1;
    std::string ip = "127.0.0.1";
    int port = 8000;
    Connection upc;

    upc.fd_ = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(upc.fd_.getFd(), SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

    upc.addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &upc.addr_.sin_addr);
    upc.addr_.sin_port = htons(port);

    if (connect(upc.fd_.getFd(), (struct sockaddr *)&upc.addr_, sizeof(upc.addr_)) < 0)
    {
        printf("error: %s\n", strerror(errno));
    }

    setnonblocking(upc.fd_.getFd());

    printf("end\n");
}