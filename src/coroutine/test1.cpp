#include "co.h"
#include "co_cond.h"
#include "co_epoll.h"
#include "co_timer.h"
#include "coctx.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

int quit = 0;

cond_waitlist_t *cond;

void *produce(void *)
{
    printf("producer begin\n");
    int cnt = 0;
    while (quit != 1)
    {
        printf("producing %d\n", cnt++);
        cond_signal(cond);

        co_poll_inner(co_get_curr_thread_env()->epoller, NULL, 0, 1000, NULL);
    }
    quit++;
    cond_signal(cond);
    printf("producer ends, quit:%d\n",quit);
}

void *consume(void *)
{
    printf("consumer begin\n");
    while (quit != 2)
    {
        cond_wait(cond);
        printf("consuming\n");
    }
    quit++;
    printf("consumer ends, quit:%d\n",quit);
}

void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        quit = 1;
        break;
    }
}

int init_signals()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = signal_handler;
    sa.sa_flags |= SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        return -1;
    }
    return 0;
}

int main()
{
    assert(init_signals()==0);

    cond = alloc_cond();

    co_routine_t *consumer;
    co_routine_t *producer;

    co_create(&consumer, consume, NULL);
    co_create(&producer, produce, NULL);

    co_resume(consumer);
    co_resume(producer);

    co_event_loop(co_get_epoll_ct(), NULL, NULL, &quit);

    free_cond(cond);

    co_release(consumer);
    co_release(producer);

    clean_thread_env();

    printf("end\n");
}