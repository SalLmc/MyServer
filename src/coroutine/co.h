#ifndef CO_H
#define CO_H

#include <pthread.h>
#include <stdint.h>
#include <sys/poll.h>

#include "co_epoll.h"
#include "coctx.h"

struct co_routine_t;
struct co_routine_env_t;
struct stack_mem_t;

typedef int (*pfn_co_eventloop_t)(void *);
typedef void *(*pfn_co_routine_t)(void *);
typedef int (*poll_pfn_t)(pollfd fds[], nfds_t nfds, int timeout_ms);

struct co_routine_env_t
{
    co_routine_t *call_stack[128];
    int call_stack_size;
    co_epoll_t *epoller;
};

struct stack_mem_t
{
    co_routine_t *occupy_co;
    int stack_size;
    char *stack_bp;
    char *stack_buffer;
};

struct co_routine_t
{
    co_routine_env_t *env;

    pfn_co_routine_t pfn;
    void *arg;

    coctx_t ctx;

    char c_start;
    char c_end;
    char c_is_main;

    void *pv_env;

    stack_mem_t *stack_mem;

    char *stack_sp;
};

struct co_poll_t;

struct co_poll_item_t : public timeout_item_t
{
    pollfd *self;
    co_poll_t *poller;
    epoll_event epevents;
};

struct co_poll_t : public timeout_item_t
{
    pollfd *fds;
    nfds_t nfds;

    co_poll_item_t *poll_items;

    int has_event_triggered;
    int epollfd;
    int raise_cnt;
};

int co_create(co_routine_t **ppco, pfn_co_routine_t pfn, void *arg);
void co_resume(co_routine_t *co);
void co_yield_env(co_routine_env_t *env);
void co_yield_ct();
void co_release(co_routine_t *co);

co_routine_env_t *co_get_curr_thread_env();
co_epoll_t *co_get_epoll_ct();
co_routine_t *co_get_curr_routine();

void co_event_loop(co_epoll_t *ctx, pfn_co_eventloop_t pfn, void *arg);

int co_poll_inner(co_epoll_t *epoller, pollfd fds[], nfds_t nfds, int timeout_ms, poll_pfn_t pollfunc);

#endif