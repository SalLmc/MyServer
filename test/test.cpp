#include "../src/core/core.h"
#include "../src/utils/utils_declaration.h"

int main()
{
    std::string ip = "127.0.0.1";
    int port = 80;
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
        return 1;
    }

    if (setnonblocking(c.fd_.getFd()) < 0)
    {
        return 1;
    }

    std::string request = "GET http://cn.bing.com/s HTTP/1.1\r\n"
                          "Host: www.example.com\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    c.writeBuffer_.append(request);
    c.writeBuffer_.bufferSend(c.fd_.getFd(), 0);

    sleep(1);

    c.readBuffer_.bufferRecv(c.fd_.getFd(), 0);

    for (auto &x : c.readBuffer_.nodes_)
    {
        printf("%s", x.toStringAll().c_str());
    }
}