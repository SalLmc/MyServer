#include "co_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/time.h>

timeout_t::timeout_t(int timer_size)
{
    size = timer_size;
    items = (timeout_link_t *)calloc(1, sizeof(timeout_link_t) * size);
    start = get_tick_ms();
    start_idx = 0;
}

timeout_t::~timeout_t()
{
    if (items != NULL)
    {
        free(items);
    }
}

void timeout_t::timeout_init(int timer_size)
{
    if (items != NULL)
    {
        free(items);
    }
    size = timer_size;
    items = (timeout_link_t *)calloc(1, sizeof(timeout_link_t) * size);
    start = get_tick_ms();
    start_idx = 0;
}

unsigned long long get_tick_ms()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    unsigned long long u = now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
}

// success -> return 0
int add_timeout(timeout_t *timer, timeout_item_t *item, unsigned long long nowtime)
{
    if (timer->start == 0)
    {
        timer->start = nowtime;
        timer->start_idx = 0;
    }

    if (nowtime < timer->start)
    {
        return __LINE__;
    }
    if (item->expire_time < nowtime)
    {
        return __LINE__;
    }

    int diff = item->expire_time - timer->start;

    if (diff >= timer->size)
    {
        return __LINE__;
    }

    int idx = (timer->start_idx + diff) % timer->size;
    timeout_link_t *addto = &timer->items[idx];

    add_tail(addto, item);

    return 0;
}

// @param timer (in)
// @param nowtime (in)
// @param result (out)
void take_all_timeout(timeout_t *timer, unsigned long long nowtime, timeout_link_t *result)
{
    if (timer->start == 0)
    {
        timer->start = nowtime;
        timer->start_idx = 0;
    }

    if (nowtime < timer->start)
    {
        return;
    }

    int cnt = nowtime - timer->start + 1;
    if (cnt > timer->size)
    {
        cnt = timer->size;
    }

    if (cnt < 0)
    {
        return;
    }

    for (int i = 0; i < cnt; i++)
    {
        int idx = (timer->start_idx + i) % timer->size;
        timeout_link_t *toadd = &timer->items[idx];
        join<timeout_link_t, timeout_item_t>(result, toadd);
    }
    timer->start = nowtime;
    timer->start_idx += cnt - 1;
}
