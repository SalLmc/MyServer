#include "co_cond.h"
#include <assert.h>

void cond_signal_handler(timeout_item_t *item)
{
    co_resume((co_routine_t *)item->arg);
}

cond_waitlist_t *alloc_cond()
{
    return (cond_waitlist_t *)calloc(1, sizeof(cond_waitlist_t));
}

int cond_wait(cond_waitlist_t *cond, unsigned long long time_ms)
{
    co_routine_env_t *env = co_get_curr_thread_env();
    co_routine_t *curr = env->call_stack[env->call_stack_size - 1];
    assert(curr != NULL);

    cond_waitlist_item_t *citem = (cond_waitlist_item_t *)calloc(1, sizeof(cond_waitlist_item_t));
    citem->timeout.arg = curr;
    citem->timeout.handler_pfn = cond_signal_handler;

    if (time_ms > 0)
    {
        auto nowtime = get_tick_ms();
        citem->timeout.expire_time = time_ms + nowtime;
        int ret = add_timeout(co_get_epoll_ct()->timer, &citem->timeout, nowtime);
        if (ret != 0)
        {
            free(citem);
            return ret;
        }
    }

    add_tail(cond, citem);
    co_yield_ct(); // switch to another coroutine

    // wakeup again
    remove_from_link<cond_waitlist_t, cond_waitlist_item_t>(citem);
    free(citem);
    return 0;
}

int cond_signal(cond_waitlist_t *cond)
{
    cond_waitlist_item_t *citem = cond->head;
    if (citem == NULL)
    {
        return 0;
    }
    pop_head<cond_waitlist_t, cond_waitlist_item_t>(cond);

    remove_from_link<timeout_link_t, timeout_item_t>(&citem->timeout);
    add_tail(co_get_epoll_ct()->list_active, &citem->timeout);
    return 0;
}

void free_cond(cond_waitlist_t *cond)
{
    free(cond);
}
