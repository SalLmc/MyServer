#include "co_timer.h"

#include <stdlib.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/time.h>

unsigned long long get_tick_ms()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    unsigned long long u = now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
}

timeout_t *alloc_timer(int timer_size)
{
    timeout_t *lp = (timeout_t *)calloc(1, sizeof(timeout_t));

    lp->size = timer_size;
    lp->items = (timeout_link_t *)calloc(1, sizeof(timeout_link_t) * lp->size);

    lp->start = get_tick_ms();
    lp->start_idx = 0;

    return lp;
}

void free_timer(timeout_t *thistimer)
{
    free(thistimer->items);
    free(thistimer);
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

    add_tail(addto,item);
    // if (!item->link)
    // {
    //     if (addto->tail)
    //     {
    //         addto->tail->next = item;
    //         item->next = NULL;
    //         item->prev = addto->tail;
    //         addto->tail = item;
    //     }
    //     else
    //     {
    //         addto->head = addto->tail = item;
    //         item->next = item->prev = NULL;
    //     }
    //     item->link = addto;
    // }
    
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
        join<timeout_link_t,timeout_item_t>(result,toadd);
        // if (toadd->head)
        // {
        //     timeout_item_t *now = toadd->head;
        //     while (now)
        //     {
        //         now->link = result;
        //         now = now->next;
        //     }
        //     now = toadd->head;
        //     if (result->tail)
        //     {
        //         result->tail->next = now;
        //         now->prev = result->tail;
        //         result->tail = toadd->tail;
        //     }
        //     else
        //     {
        //         result->head = toadd->head;
        //         result->tail = toadd->tail;
        //     }
        //     toadd->head = toadd->tail = NULL;
        // }
    }
    timer->start = nowtime;
    timer->start_idx += cnt - 1;
}
