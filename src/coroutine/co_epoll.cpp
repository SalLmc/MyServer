#include "co_epoll.h"

int co_epoll_wait(int epfd, struct co_epoll_res *events, int maxevents, int timeout)
{
    return epoll_wait(epfd, events->events, maxevents, timeout);
}

int co_epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev)
{
    return epoll_ctl(epfd, op, fd, ev);
}

int co_epoll_create(int size)
{
    return epoll_create(size);
}

co_epoll_res *co_epoll_res_alloc(int n)
{
    co_epoll_res *now = (co_epoll_res *)malloc(sizeof(co_epoll_res));
    now->size = n;
    now->events = (epoll_event *)calloc(1, n * sizeof(epoll_event));
    return now;
}

void co_epoll_res_free(struct co_epoll_res *ptr)
{
    if(!ptr)
        return ;
    if(ptr->events)
        free(ptr->events);
    free(ptr);
}

co_epoll_t *alloc_epoll()
{
    co_epoll_t *now = (co_epoll_t *)calloc(1, sizeof(co_epoll_t));
    now->epollfd = co_epoll_create(co_epoll_t::_EPOLL_SIZE);

    now->timer = alloc_timer();
    now->list_active = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));
    now->list_timeout = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));

    return now;
}

void free_epoll( co_epoll_t *ctx )
{
	if( ctx )
	{
		free( ctx->list_active );
		free( ctx->list_timeout );
		free_timer(ctx->timer);
		co_epoll_res_free( ctx->result );
	}
	free( ctx );
}