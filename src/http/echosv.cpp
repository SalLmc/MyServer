#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../core/core.h"
#include "../event/epoller.h"

#define READ_EVENT 1
#define WRITE_EVENT 2

bool isStop;
int sigPipe[2];

ConnectionPool connectionPool;
Epoller epoller;

int recvPrint(Event *ev)
{
    int len = ev->c->readBuffer_.readFd(ev->c->fd_, &errno);
    if (len == 0)
    {
        printf("client close connection\n");
        connectionPool.recoverConnection(ev->c);
        return 1;
    }
    else if (len < 0)
    {
        printf("errno:%d\n", errno);
        connectionPool.recoverConnection(ev->c);
        return 1;
    }
    printf("recv len:%d from client:%s\n", len, ev->c->readBuffer_.allToStr().c_str());
    ev->c->writeBuffer_.append(ev->c->readBuffer_.retrieveAllToStr());
    return 0;
}

int echoPrint(Event *ev)
{
    ev->c->writeBuffer_.writeFd(ev->c->fd_, &errno);
    ev->c->writeBuffer_.retrieveAll();
    return 0;
}

int newConnection(Event *ev)
{
    Connection *newc = connectionPool.getNewConnection();
    assert(newc != NULL);

    sockaddr_in *addr = &newc->addr_;
    socklen_t len = sizeof(*addr);

    newc->fd_ = accept(ev->c->fd_, (sockaddr *)addr, &len);
    assert(newc->fd_ > 0);

    setnonblocking(newc->fd_);

    newc->read_.c = newc->write_.c = newc;

    newc->read_.handler = recvPrint;
    newc->write_.handler = echoPrint;

    epoller.addFd(newc->fd_, EPOLLIN | EPOLLOUT, newc);

    printf("new connection\n");
    return 0;
}

Connection *addListen(int port)
{
    Connection *listenC = connectionPool.getNewConnection();

    assert(listenC != NULL);

    listenC->addr_.sin_addr.s_addr = INADDR_ANY;
    listenC->addr_.sin_family = AF_INET;
    listenC->addr_.sin_port = htons(port);
    listenC->read_.c = listenC->write_.c = listenC;
    listenC->read_.handler = newConnection;

    listenC->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenC->fd_ > 0);

    int reuse = 1;
    setsockopt(listenC->fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setnonblocking(listenC->fd_);

    assert(bind(listenC->fd_, (sockaddr *)&listenC->addr_, sizeof(listenC->addr_)) == 0);

    assert(listen(listenC->fd_, 5) == 0);

    return listenC;
}

void sigHandler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sigPipe[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void addSig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sigHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int sigDiffer(Event *ev)
{
    int sig;
    recv(ev->c->fd_, &sig, 1, 0);
    if ((sig & 0xf) == SIGINT)
    {
        isStop = 1;
    }
}

void handleSignal()
{
    assert(socketpair(PF_UNIX, SOCK_STREAM, 0, sigPipe) != -1);

    setnonblocking(sigPipe[0]);
    setnonblocking(sigPipe[1]);

    addSig(SIGINT);

    Connection *signalC = connectionPool.getNewConnection();
    signalC->fd_ = sigPipe[0];
    signalC->read_.handler = sigDiffer;
    signalC->read_.c = signalC;

    epoller.addFd(signalC->fd_, EPOLLIN, signalC);
}

int main(int argc, char *argv[])
{
    nanolog::initialize(nanolog::NonGuaranteedLogger(10), "./log/", "slog", 1);

    if (argc < 2)
    {
        printf("usage: %s port_number\n", basename(argv[0]));
        return 1;
    }
    int port = atoi(argv[1]);

    // setnonblocking(epoller.getFd());

    Connection *listenC = addListen(port);
    assert(epoller.addFd(listenC->fd_, EPOLLIN, listenC));

    handleSignal();

    isStop = 0;

    while (!isStop)
    {
        epoller.processEvents();
    }

    printf("server close\n");
}