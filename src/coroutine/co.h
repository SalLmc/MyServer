#ifndef CO_H
#define CO_H

#include <pthread.h>
#include <stdint.h>
#include <sys/poll.h>

#include "co_epoll.h"
#include "coctx.h"

typedef int (*co_eventloop_fn)(void *);
typedef void *(*co_routine_fn)(void *);
typedef int (*poll_fn)(pollfd fds[], nfds_t nfds, int timeout_ms);

class co_routine_env_t;
class stack_mem_t;

class co_routine_t
{
  public:
    co_routine_t() = default;
    co_routine_t(co_routine_env_t *envv, co_routine_fn fn, void *argg);
    ~co_routine_t();

    co_routine_env_t *env = NULL;

    co_routine_fn func;
    void *arg = NULL;

    coctx_t ctx;

    char c_start;
    char c_end;
    char c_is_main;

    void *pv_env = NULL;

    stack_mem_t *stack_mem = NULL;

    char *stack_sp = NULL;
};

class co_routine_env_t
{
  public:
    co_routine_env_t();
    ~co_routine_env_t();
    co_routine_t *call_stack[128];
    int call_stack_size;
    co_epoll_t *epoller;
};

class stack_mem_t
{
  public:
    stack_mem_t() = default;
    stack_mem_t(unsigned int size);
    ~stack_mem_t();

    co_routine_t *occupy_co = NULL;
    unsigned int stack_size = 0;
    char *stack_bp = NULL;
    char *stack_buffer = NULL;
};

struct co_poll_item_t;

struct co_poll_t : public timeout_item_t
{
    pollfd *fds;
    nfds_t nfds;

    co_poll_item_t *poll_items;

    int has_event_triggered;
    int epollfd;
    int raise_cnt;
};

struct co_poll_item_t : public timeout_item_t
{
    pollfd *self;
    co_poll_t *poller;
    epoll_event epevents;
};

int co_create(co_routine_t **ppco, co_routine_fn func, void *arg);
void co_resume(co_routine_t *co);
void co_yield_env(co_routine_env_t *env);
void co_yield_ct();
void co_release(co_routine_t *co);

co_routine_env_t *co_get_curr_thread_env();
void clean_thread_env();
co_epoll_t *co_get_epoll_ct();
co_routine_t *co_get_curr_routine();

void co_event_loop(co_epoll_t *ctx, co_eventloop_fn func, void *arg, int *quit);

int co_poll_inner(co_epoll_t *epoller, pollfd fds[], nfds_t nfds, int timeout_ms, poll_fn pollfunc);

#endif