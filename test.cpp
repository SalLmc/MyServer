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
#include <sys/time.h>
#include <unordered_map>

unsigned long long getTickMs()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    unsigned long long u = now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
}

class Connection
{
  public:
    int fd_;
    sockaddr_in addr_;
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    int port = atoi(argv[2]);

    Connection c;

    inet_pton(AF_INET, argv[1], &c.addr_.sin_addr);
    c.addr_.sin_family = AF_INET;
    c.addr_.sin_port = htons(port);
    c.fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(c.fd_ >= 0);

    assert(connect(c.fd_, (sockaddr *)&c.addr_, sizeof(c.addr_)) != -1);

    auto time0 = getTickMs();

    char buffer[] = "HELLO";

    send(c.fd_, buffer, sizeof(buffer), 0);

    char recvBuffer[1024];
    memset(recvBuffer, 0, sizeof(recvBuffer));
    recv(c.fd_, recvBuffer, 1000, 0);

    auto delta = getTickMs() - time0;
    if (delta >= 5000)
    {
        printf("host unreachable\n");
    }
    else
    {
        printf("connect OK\n");
    }
}