#include "http.h"

RequestPool::RequestPool()
{
    flags = 0;
    rPool_ = new Request *[POOLSIZE];
    for (int i = 0; i < POOLSIZE; i++)
    {
        rPool_[i] = new Request;
        rPool_[i]->idx_ = -1;
    }
}

RequestPool::~RequestPool()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        delete rPool_[i];
    }
    delete rPool_;
}

Request *RequestPool::getNewRequest()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        if (!(flags >> i))
        {
            flags |= (1 << i);
            rPool_[i]->idx_ = i;
            return rPool_[i];
        }
    }
    return NULL;
}

void RequestPool::recoverRequest(Request *r)
{
    uint8_t recover = ~(1 << r->idx_);
    flags &= recover;
    r->idx_ = -1;
    r->c = NULL;
    // r->headerBuffer_.retrieveAll();
}