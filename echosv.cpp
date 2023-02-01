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

#include "src/core/core.h"
#include "src/event/epoller.h"
#include "src/util/utils_declaration.h"
#include "src/http/http.h"
#include "src/global.h"

bool isStop;
int sigPipe[2];

std::unordered_map<std::string, std::string> mp;

int recvPrint(Event *ev)
{
    int len = ev->c->readBuffer_.readFd(ev->c->fd_.getFd(), &errno);
    if (len == 0)
    {
        printf("client close connection\n");
        pool.recoverConnection(ev->c);
        return 1;
    }
    else if (len < 0)
    {
        printf("errno:%d\n", errno);
        pool.recoverConnection(ev->c);
        return 1;
    }
    printf("recv len:%d from client:%s\n", len, ev->c->readBuffer_.allToStr().c_str());
    ev->c->writeBuffer_.append(ev->c->readBuffer_.retrieveAllToStr());
    return 0;
}

int echoPrint(Event *ev)
{
    ev->c->writeBuffer_.writeFd(ev->c->fd_.getFd(), &errno);
    ev->c->writeBuffer_.retrieveAll();
    return 0;
}

int newConnection(Event *ev)
{
    Connection *newc = pool.getNewConnection();
    assert(newc != NULL);

    sockaddr_in *addr = &newc->addr_;
    socklen_t len = sizeof(*addr);

    newc->fd_ = accept(ev->c->fd_.getFd(), (sockaddr *)addr, &len);
    assert(newc->fd_.getFd() > 0);

    setnonblocking(newc->fd_.getFd());

    newc->read_.c = newc->write_.c = newc;

    newc->read_.handler = recvPrint;
    newc->write_.handler = echoPrint;

    epoller.addFd(newc->fd_.getFd(), EPOLLIN | EPOLLOUT, newc);

    printf("NEW CONNECTION\n");
    return 0;
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
    recv(ev->c->fd_.getFd(), &sig, 1, 0);
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

    Connection *signalC = pool.getNewConnection();
    signalC->fd_ = sigPipe[0];
    signalC->read_.handler = sigDiffer;
    signalC->read_.c = signalC;

    epoller.addFd(signalC->fd_.getFd(), EPOLLIN, signalC);
}

int main(int argc, char *argv[])
{
    assert(getOption(argc, argv, &mp) == 0);
    nanolog::initialize(nanolog::NonGuaranteedLogger(10), "./log/", "log", 1);

    Cycle cycle;
    cycle.pool_ = &pool;

    Connection *listenC = addListen(&cycle, std::stoi(mp["port"]));
    listenC->read_.handler=newConnection;

    assert(epoller.addFd(listenC->fd_.getFd(), EPOLLIN, listenC));

    handleSignal();

    isStop = 0;

    while (!isStop)
    {
        epoller.processEvents();
    }

    printf("server close\n");
}