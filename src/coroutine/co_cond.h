#include "co.h"
#include "co_timer.h"

struct cond_waitlist_t;

struct cond_waitlist_item_t
{
    cond_waitlist_t *link;
    cond_waitlist_item_t *prev;
    cond_waitlist_item_t *next;
    timeout_item_t timeout;
};

struct cond_waitlist_t
{
    cond_waitlist_item_t *head;
    cond_waitlist_item_t *tail;
};

void cond_signal_handler(timeout_item_t *item);

cond_waitlist_t *alloc_cond();
void free_cond(cond_waitlist_t *cond);

// @param time_ms=0 means blocking
int cond_wait(cond_waitlist_t *cond, unsigned long long time_ms = 0);
int cond_signal(cond_waitlist_t *cond);