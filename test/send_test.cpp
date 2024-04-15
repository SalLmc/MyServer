#include "../src/core/core.h"
#include "../src/event/event.h"
#include "../src/log/logger.h"
#include "../src/utils/utils.h"

// need to define these global variables
Server *serverPtr;
bool enableLogger = 1;
Level loggerLevel = Level::INFO;

extern ServerAttribute defaultAttr;

int recvFunc(Event *ev)
{
    auto c = ev->c_;
    int n = c->readBuffer_.bufferRecv(c->fd_.get(), 0);

    if (n <= 0)
    {
        LOG_INFO << "errno: " << strerror(errno);
        if (n == 0)
        {
            LOG_INFO << "Connection closed";
            exit(0);
        }
        return ERROR;
    }

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    for (auto &x : c->readBuffer_.nodes_)
    {
        printf("%s", x.toStringAll().c_str());
    }

    printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

    return OK;
}

int sendFunc(Event *ev)
{
    auto c = ev->c_;
    int ret = 0;
    for (; c->writeBuffer_.allRead() != 1;)
    {
        ret = c->writeBuffer_.bufferSend(c->fd_.get(), 0);

        if (c->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                return AGAIN;
            }
            else
            {
                LOG_WARN << "Send error, errno: " << strerror(errno);
                exit(1);
            }
        }
        else if (ret == 0)
        {
            LOG_WARN << "Client close connection";
            exit(1);
        }
        else
        {
            continue;
        }
    }
    LOG_INFO << "Sent";
    serverPtr->multiplexer_->modFd(c->fd_.get(), Events(IN | ET), c);
    c->read_.handler_ = recvFunc;

    return recvFunc(&c->read_);
}

int main()
{
    Server server(new Logger("../log", "test"));
    serverPtr = &server;

    server.initEvent(1);

    Connection c;
    c.fd_ = socket(AF_INET, SOCK_STREAM, 0);
    c.addr_.sin_family = AF_INET;
    inet_pton(AF_INET, "139.199.176.139", &c.addr_.sin_addr);
    c.addr_.sin_port = htons(8083);

    if (connect(c.fd_.get(), (const sockaddr *)&c.addr_, sizeof(c.addr_)) != 0)
    {
        LOG_INFO << "connect errno: " << strerror(errno);
    }

    setnonblocking(c.fd_.get());

    c.writeBuffer_.append("GET / HTTP/1.1\r\n");
    c.writeBuffer_.append("Host: localhost:8000\r\n");
    c.writeBuffer_.append("User-Agent: curl/7.81.0\r\n");
    c.writeBuffer_.append("Accept: */*\r\n\r\n");

    c.write_.handler_ = sendFunc;

    server.multiplexer_->addFd(c.fd_.get(), Events(ET | OUT), &c);

    while (1)
    {
        server.eventLoop();
    }
}