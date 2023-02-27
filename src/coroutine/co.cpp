#include "co.h"

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <errno.h>
#include <poll.h>
#include <sys/time.h>

#include <assert.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

// 128KB
#define STACK_SIZE 131072

co_routine_env_t *arr_env_thread[204800] = {0};

stack_mem_t::stack_mem_t(unsigned int size)
{
    occupy_co = NULL;
    stack_size = size;
    stack_buffer = (char *)calloc(1, stack_size);
    stack_bp = stack_buffer + stack_size;
}

stack_mem_t::~stack_mem_t()
{
    if (stack_buffer != NULL)
    {
        free(stack_buffer);
    }
}

co_routine_env_t::co_routine_env_t()
{
    memset(call_stack, 0, 128 * (sizeof(co_routine_t *)));
    call_stack_size = 0;
    epoller = new co_epoll_t();
}

co_routine_env_t::~co_routine_env_t()
{
    delete epoller;
    delete call_stack[0];
}

co_routine_t::co_routine_t(co_routine_env_t *envv, co_routine_fn fn, void *argg)
{
    env = envv;
    func = fn;
    arg = argg;

    c_start = 0;
    c_end = 0;
    c_is_main = 0;

    stack_mem = new stack_mem_t(STACK_SIZE);
    ctx.ss_sp = stack_mem->stack_buffer;
    ctx.ss_size = STACK_SIZE;
}

co_routine_t::~co_routine_t()
{
    delete stack_mem;
}

void co_init_curr_thread_env()
{
    pid_t pid = syscall(__NR_gettid);

    co_routine_env_t *env = new co_routine_env_t();

    arr_env_thread[pid] = env;

    co_routine_t *self = new co_routine_t(env, NULL, NULL);
    self->c_is_main = 1;

    // the reason the main coroutine needs coctx_init is because
    // when the main coroutine call function coctx_swap
    // coctx_swap save current stack (OS real stack) to main's coctx_t regs
    // the reason other coroutines don't call coctx_init is because
    // they need to use coctx_t::ss_sp & ss_size to calculate where to swap to
    coctx_init(&self->ctx);

    env->call_stack[env->call_stack_size++] = self;
}

co_routine_env_t *co_get_curr_thread_env()
{
    return arr_env_thread[syscall(__NR_gettid)];
}

void clean_thread_env()
{
    delete arr_env_thread[syscall(__NR_gettid)];
}

co_routine_t *co_get_curr_routine()
{
    auto env = co_get_curr_thread_env();
    if (env == NULL)
        return NULL;
    return env->call_stack[env->call_stack_size - 1];
}

void co_swap(co_routine_t *curr, co_routine_t *pending_co)
{
    coctx_swap(&(curr->ctx), &(pending_co->ctx));
}

void co_yield_env(co_routine_env_t *env)
{
    co_routine_t *last = env->call_stack[env->call_stack_size - 2];
    co_routine_t *curr = env->call_stack[env->call_stack_size - 1];
    env->call_stack_size--;

    co_swap(curr, last);
};

void co_yield_ct()
{
    co_routine_env_t *env = co_get_curr_thread_env();

    co_routine_t *last = env->call_stack[env->call_stack_size - 2];
    co_routine_t *curr = env->call_stack[env->call_stack_size - 1];
    env->call_stack_size--;

    co_swap(curr, last);
}

co_epoll_t *co_get_epoll_ct()
{
    return co_get_curr_thread_env()->epoller;
}

static int co_routine_func_wrapper(co_routine_t *co, void *extra)
{
    if (co->func)
    {
        co->func(co->arg);
    }

    // 标识该协程已经结束
    co->c_end = 1;

    co_yield_env(co->env);
    return 0;
}

int co_create(co_routine_t **ppco, co_routine_fn pfn, void *arg)
{
    if (!co_get_curr_thread_env())
    {
        co_init_curr_thread_env();
    }
    co_routine_t *co = new co_routine_t(co_get_curr_thread_env(), pfn, arg);
    *ppco = co;
    return 0;
}

// this coroutine auto yields himself when his function completes
void co_resume(co_routine_t *co)
{
    co_routine_env_t *env = co->env;

    co_routine_t *now = env->call_stack[env->call_stack_size - 1];

    if (!co->c_start)
    {
        coctx_make(&co->ctx, (coctx_pfn_t)co_routine_func_wrapper, co, 0);
        co->c_start = 1;
    }

    env->call_stack[env->call_stack_size++] = co;

    co_swap(now, co);
};

void co_free(co_routine_t *co)
{
    delete co;
}

void co_release(co_routine_t *co)
{
    co_free(co);
}

void co_event_loop(co_epoll_t *ctx, co_eventloop_fn pfn, void *arg, int *quit)
{
    if (!ctx->result)
    {
        ctx->result = new co_epoll_res(co_epoll_t::_EPOLL_SIZE);
    }

    co_epoll_res *result = ctx->result;

    for (; *quit != 3;)
    {
        // timeout 1ms
        int ret = co_epoll_wait(ctx->epollfd, result, co_epoll_t::_EPOLL_SIZE, 1);

        timeout_link_t *active = ctx->list_active;
        timeout_link_t *timeout = ctx->list_timeout;

        memset(timeout, 0, sizeof(timeout_link_t));

        // prework active events
        for (int i = 0; i < ret; i++)
        {
            timeout_item_t *item = (timeout_item_t *)result->events[i].data.ptr;

            if (item->prework)
            {
                item->prework(item, result->events[i], active);
            }
            else
            {
                add_tail(active, item);
            }
        }

        // timeout events
        unsigned long long nowtime = get_tick_ms();

        take_all_timeout(ctx->timer, nowtime, timeout);

        timeout_item_t *now = timeout->head;
        while (now)
        {
            now->is_timeout = true;
            now = now->next;
        }

        join<timeout_link_t, timeout_item_t>(active, timeout);

        // active events
        now = active->head;
        while (now)
        {
            pop_head<timeout_link_t, timeout_item_t>(active);
            if (now->handler)
            {
                now->handler(now);
            }
            now = active->head;
        }

        // co_event_loop
        if (pfn)
        {
            if (-1 == pfn(arg))
            {
                break;
            }
        }
    }
}

static uint32_t poll2epoll(short events)
{
    uint32_t e = 0;
    if (events & POLLIN)
        e |= EPOLLIN;
    if (events & POLLOUT)
        e |= EPOLLOUT;
    if (events & POLLHUP)
        e |= EPOLLHUP;
    if (events & POLLERR)
        e |= EPOLLERR;
    if (events & POLLRDNORM)
        e |= EPOLLRDNORM;
    if (events & POLLWRNORM)
        e |= EPOLLWRNORM;
    return e;
}
static short epoll2poll(uint32_t events)
{
    short e = 0;
    if (events & EPOLLIN)
        e |= POLLIN;
    if (events & EPOLLOUT)
        e |= POLLOUT;
    if (events & EPOLLHUP)
        e |= POLLHUP;
    if (events & EPOLLERR)
        e |= POLLERR;
    if (events & EPOLLRDNORM)
        e |= POLLRDNORM;
    if (events & EPOLLWRNORM)
        e |= POLLWRNORM;
    return e;
}

void poller_handler(timeout_item_t *now)
{
    co_resume((co_routine_t *)now->arg);
}

void pollevent_prework(timeout_item_t *item, epoll_event &ev, timeout_link_t *active)
{
    co_poll_item_t *now = (co_poll_item_t *)item;
    now->self->revents = epoll2poll(ev.events);

    co_poll_t *poller = now->poller;
    poller->raise_cnt++;

    if (!poller->has_event_triggered)
    {
        poller->has_event_triggered = 1;
        // if there are events captured after co_epoll_wait in co_eventloop
        // then remove poller from timeout link (added in co_poll_inner) so that co_poll_inner can return the amount of
        // events triggered in one co_epoll_wait
        remove_from_link<timeout_link_t, timeout_item_t>(poller);
        // add poller to active list waiting to resume
        add_tail(active, poller);
    }
}

int co_poll_inner(co_epoll_t *epoller, pollfd fds[], nfds_t nfds, int timeout_ms, poll_fn pollfunc)
{
    if (timeout_ms > timeout_item_t::eMaxTimeout)
    {
        timeout_ms = timeout_item_t::eMaxTimeout;
    }

    int epfd = epoller->epollfd;

    co_poll_t *poller = (co_poll_t *)calloc(1, sizeof(co_poll_t));
    poller->epollfd = epfd;
    poller->fds = (pollfd *)calloc(nfds, sizeof(pollfd));
    poller->nfds = nfds;
    poller->poll_items = (co_poll_item_t *)calloc(nfds, sizeof(co_poll_item_t));

    poller->arg = co_get_curr_routine();
    poller->handler = poller_handler;

    // poll events
    for (nfds_t i = 0; i < nfds; i++)
    {
        poller->poll_items[i].self = &poller->fds[i];
        poller->poll_items[i].poller = poller;
        poller->poll_items[i].prework = pollevent_prework;

        epoll_event &ev = poller->poll_items[i].epevents;

        if (fds[i].fd > -1)
        {
            ev.data.ptr = &poller->poll_items[i];
            ev.events = poll2epoll(fds[i].events);

            int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev);

            if (ret < 0 && errno == EPERM && nfds == 1 && pollfunc != NULL)
            {
                free(poller->poll_items);
                poller->poll_items = NULL;
                free(poller->fds);
                free(poller);

                return pollfunc(fds, nfds, timeout_ms);
            }
        }
    }

    // timeout
    auto nowtime = get_tick_ms();
    poller->expire_time = nowtime + timeout_ms;

    assert(add_timeout(epoller->timer, poller, nowtime) == 0);

    co_yield_ct();

    remove_from_link<timeout_link_t, timeout_item_t>(poller);

    for (nfds_t i = 0; i < nfds; i++)
    {
        int fd = fds[i].fd;
        if (fd > -1)
        {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &poller->poll_items[i].epevents);
        }
        fds[i].revents = poller->fds[i].revents;
    }

    int cnt = poller->raise_cnt;
    free(poller->poll_items);
    poller->poll_items = NULL;
    free(poller->fds);
    free(poller);

    return cnt;
}