#ifndef CO_TIMER_H
#define CO_TIMER_H

#include <assert.h>
#include <stdlib.h>
#include <sys/epoll.h>

struct timeout_item_t;
struct timeout_link_t;

typedef void (*prework_t)(timeout_item_t *, epoll_event &ev, timeout_link_t *active);
typedef void (*handler_t)(timeout_item_t *);

struct timeout_item_t
{
    enum
    {
        eMaxTimeout = 40 * 1000 // 40s
    };

    timeout_item_t *prev;
    timeout_item_t *next;

    timeout_link_t *link;

    unsigned long long expire_time;

    prework_t prework; // call in eventloop
    handler_t handler; // call in eventloop

    void *arg; // arg of prework & handler

    bool is_timeout;
};

struct timeout_link_t
{
    timeout_item_t *head;
    timeout_item_t *tail;
};

class timeout_t
{
  public:
    timeout_t() = default;
    timeout_t(int timer_size = 60000);
    ~timeout_t();

    void timeout_init(int timer_size = 60000);

    timeout_link_t *items = NULL; // is an array
    int size;                     // 60*1000 default

    unsigned long long start; // earliest time to expire
    long long start_idx;
};

unsigned long long get_tick_ms();

int add_timeout(timeout_t *timer, timeout_item_t *item, unsigned long long nowtime);
void take_all_timeout(timeout_t *timer, unsigned long long nowtime, timeout_link_t *result);

template <class TLink, class TNode> void inline add_tail(TLink *link, TNode *node)
{
    if (node->link)
    {
        return;
    }
    if (link->tail)
    {
        link->tail->next = node;
        node->next = NULL;
        node->prev = link->tail;
        link->tail = node;
    }
    else
    {
        link->head = link->tail = node;
        node->next = node->prev = NULL;
    }
    node->link = link;
}

// join b to a
template <class TLink, class TNode> void inline join(TLink *a, TLink *b)
{
    if (!b->head)
    {
        return;
    }
    TNode *now = b->head;
    while (now)
    {
        now->link = a;
        now = now->next;
    }
    now = b->head;
    if (a->tail)
    {
        a->tail->next = now;
        now->prev = a->tail;
        a->tail = b->tail;
    }
    else
    {
        a->head = b->head;
        a->tail = b->tail;
    }
    b->head = b->tail = NULL;
}

template <class TLink, class TNode> void inline pop_head(TLink *link)
{
    if (!link->head)
    {
        return;
    }
    TNode *lp = link->head;
    if (link->head == link->tail)
    {
        link->head = link->tail = NULL;
    }
    else
    {
        link->head = link->head->next;
    }

    lp->prev = lp->next = NULL;
    lp->link = NULL;

    if (link->head)
    {
        link->head->prev = NULL;
    }
}

template <class TLink, class TNode> void inline remove_from_link(TNode *ap)
{
    TLink *lst = ap->link;

    if (!lst)
        return;
    assert(lst->head && lst->tail);

    if (ap == lst->head)
    {
        lst->head = ap->next;
        if (lst->head)
        {
            lst->head->prev = NULL;
        }
    }
    else
    {
        if (ap->prev)
        {
            ap->prev->next = ap->next;
        }
    }

    if (ap == lst->tail)
    {
        lst->tail = ap->prev;
        if (lst->tail)
        {
            lst->tail->next = NULL;
        }
    }
    else
    {
        ap->next->prev = ap->prev;
    }

    ap->prev = ap->next = NULL;
    ap->link = NULL;
}

#endif