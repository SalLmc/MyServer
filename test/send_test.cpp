#include "../src/core/core.h"
#include "../src/log/logger.h"

// need to define these global variables
Server *serverPtr;
bool enableLogger = 0;
Level loggerLevel = Level::INFO;

int main()
{
    Connection c;
    c.fd_ = socket(AF_INET, SOCK_STREAM, 0);
    c.addr_.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &c.addr_.sin_addr);
    c.addr_.sin_port = htons(8082);

    connect(c.fd_.get(), (const sockaddr *)&c.addr_, sizeof(c.addr_));

    c.writeBuffer_.append("GET /s+../Documents.txt#HEADER HTTP/1.1\r\n");
    c.writeBuffer_.append("Host: localhost:8082\r\n");
    c.writeBuffer_.append("User-Agent: curl/7.81.0\r\n");
    c.writeBuffer_.append("Accept: */*\r\n\r\n");

    c.writeBuffer_.bufferSend(c.fd_.get(), 0);

    c.readBuffer_.bufferRecv(c.fd_.get(), 0);

    for (auto &x : c.readBuffer_.nodes_)
    {
        printf("%s", x.toStringAll().data());
    }
}