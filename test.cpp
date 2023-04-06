#include "src/headers.h"

#include "src/util/utils_declaration.h"

int main()
{
    // int fd=open("nohup.out",O_RDONLY);
    // assert(fd>=0);
    // int fd2=open("nohup.out",O_RDONLY);
    // assert(fd2>=0);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse));
    setnonblocking(listenfd);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8081);

    bind(listenfd, (sockaddr *)&addr, sizeof(addr));
    listen(listenfd, 5);

    int epfd = epoll_create1(0);
    struct epoll_event event;
    event.data.fd = epfd;
    event.events = EPOLLIN | EPOLLEXCLUSIVE;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

    int pid = fork();
    if (pid == 0)
    {
        // int epfd = epoll_create1(0);
        // struct epoll_event event;
        // event.data.fd = epfd;
        // event.events = EPOLLIN | EPOLLEXCLUSIVE;
        // epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
        epoll_event events[1024] = {0};
        while (1)
        {
            int len = epoll_wait(epfd, events, 1024, 1);

            for (int i = 0; i < len; i++)
            {
                printf("%d %d\n", getpid(), events[i].data.fd);
            }
        }
    }
    else
    {
        // int epfd = epoll_create1(0);
        // struct epoll_event event;
        // event.data.fd = epfd;
        // event.events = EPOLLIN | EPOLLEXCLUSIVE;
        // epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
        epoll_event events[1024] = {0};
        while (1)
        {
            int len = epoll_wait(epfd, events, 1024, 1);

            for (int i = 0; i < len; i++)
            {
                printf("%d %d\n", getpid(), events[i].data.fd);
            }
        }
    }
}