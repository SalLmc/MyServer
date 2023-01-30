#include "co.h"
#include "co_cond.h"
#include "co_epoll.h"
#include "co_timer.h"
#include "coctx.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

cond_waitlist_t *cond;

void *produce(void *)
{
    printf("producer begin\n");
    int cnt=0;
    while (1)
    {
        printf("producing %d\n",cnt++);
        cond_signal(cond);

        co_poll_inner(co_get_curr_thread_env()->epoller,NULL,0,1000,NULL);        
        // timeout_item_t *item=(timeout_item_t*)calloc(1,sizeof(timeout_item_t));
        // item->expire_time=1000+get_tick_ms();
        // item->handler_pfn=cond_signal_handler;
        // item->arg=co_get_curr_routine();

        // add_timeout(co_get_epoll_ct()->timer,item,get_tick_ms());
        // co_yield_ct();
        
        // free(item);
    }
}

void *consume(void *)
{
    printf("consumer begin\n");
    while (1)
    {
        cond_wait(cond);
        printf("consuming\n");
    }
}

int main()
{
    cond = alloc_cond();

    co_routine_t *consumer;
    co_routine_t *producer;

    co_create(&consumer, consume, NULL);
    co_create(&producer, produce, NULL);

    co_resume(consumer);
    co_resume(producer);

    co_event_loop(co_get_epoll_ct(), NULL, NULL);

    free_cond(cond);
}