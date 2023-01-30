#ifndef CO_TIMER_H
#define CO_TIMER_H

#include <stdlib.h>
#include <sys/epoll.h>
#include <assert.h>

struct timeout_item_t;
struct timeout_link_t;

typedef void (*prework_pfn_t)(timeout_item_t *, epoll_event &ev, timeout_link_t *active);
typedef void (*handler_pfn_t)(timeout_item_t *);

struct timeout_item_t
{
    enum
    {
        eMaxTimeout = 40 * 1000 // 40s
    };

    timeout_item_t *prev; // 前一个元素
    timeout_item_t *next; // 后一个元素

    timeout_link_t *link; // 该链表项所属的链表

    unsigned long long expire_time;

    prework_pfn_t prework_pfn; // 预处理函数，在eventloop中会被调用
    handler_pfn_t handler_pfn; // 处理函数 在eventloop中会被调用

    void *arg; // 是handler_pfn和prework_pfn的参数

    bool is_timeout; // 是否已经超时
};

struct timeout_link_t
{
    timeout_item_t *head;
    timeout_item_t *tail;
};

struct timeout_t
{
    /*
       时间轮
       超时事件数组，总长度为iItemSize,每一项代表1毫秒，为一个链表，代表这个时间所超时的事件。

       这个数组在使用的过程中，会使用取模的方式，把它当做一个循环数组来使用，虽然并不是用循环链表来实现的
    */
    timeout_link_t *items; // 是数组
    int size;              // 默认为60*1000

    unsigned long long start; // 目前的超时管理器最早的时间
    long long start_idx;      // 目前最早的时间所对应的items上的索引
};

unsigned long long get_tick_ms();
timeout_t *alloc_timer(int timer_size = 60000);
void free_timer(timeout_t *thistimer);
int add_timeout(timeout_t *timer, timeout_item_t *item, unsigned long long nowtime);
void take_all_timeout(timeout_t *timer, unsigned long long nowtime, timeout_link_t *result);

template <class TLink, class TNode> 
void inline add_tail(TLink *link, TNode *node)
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
template <class TLink, class TNode> 
void inline join(TLink *a, TLink *b)
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

template <class TLink, class TNode>
void inline pop_head(TLink *link)
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

template <class TLink,class TNode>
void inline remove_from_link(TNode *ap)
{
	TLink *lst = ap->link;

	if(!lst) return ;
	assert( lst->head && lst->tail );

	if( ap == lst->head )
	{
		lst->head = ap->next;
		if(lst->head)
		{
			lst->head->prev = NULL;
		}
	}
	else
	{
		if(ap->prev)
		{
			ap->prev->next = ap->next;
		}
	}

	if( ap == lst->tail )
	{
		lst->tail = ap->prev;
		if(lst->tail)
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