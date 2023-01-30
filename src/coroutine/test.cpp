#include "co.h"
#include "co_cond.h"
#include "co_epoll.h"
#include "co_timer.h"
#include "coctx.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

int loop(void *)
{
    printf("looping\n");
    return 1;
}

void *hello(void *)
{
    printf("hello\n");
    return NULL;
}

void prework(timeout_item_t *, epoll_event &ev, timeout_link_t *active)
{
    printf("preworking\n");
}

void handler(timeout_item_t *)
{
    printf("handling\n");
}

int main()
{
    co_routine_t *co;
    co_create(&co, hello, NULL);
    co_resume(co);

    timeout_item_t item;
    printf("%x\n", item.link);
    memset(&item, 0, sizeof(timeout_item_t));
    printf("%x\n", item.link);
    item.expire_time = 1000 + get_tick_ms();
    item.prework_pfn = prework;
    item.handler_pfn = handler;

    auto timer = co_get_curr_thread_env()->epoller->timer;

    assert(add_timeout(timer, &item, get_tick_ms()) == 0);

    co_event_loop(co_get_curr_thread_env()->epoller, NULL, NULL);
}