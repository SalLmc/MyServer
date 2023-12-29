#include "../src/core/core.h"
#include "../src/utils/utils_declaration.h"

int main()
{
    std::string ip = "127.0.0.1";
    int port = 8000;
    int val = 1;
    Connection c;

    c.fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (c.fd_.getFd() < 0)
    {
        return 1;
    }

    if (setsockopt(c.fd_.getFd(), SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0)
    {
        return 1;
    }

    c.addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &c.addr_.sin_addr);
    c.addr_.sin_port = htons(port);

    if (connect(c.fd_.getFd(), (struct sockaddr *)&c.addr_, sizeof(c.addr_)) < 0)
    {
        printf("%s\n", strerror(errno));
        return 1;
    }

    if (setnonblocking(c.fd_.getFd()) < 0)
    {
        return 1;
    }

    if (send(c.fd_.getFd(), "123", 3, 0) < 0)
    {
        printf("%s\n", strerror(errno));
    }

    printf("HERE\n");
}