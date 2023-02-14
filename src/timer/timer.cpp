#include "timer.h"
#include "../util/utils_declaration.h"
#include <assert.h>

void HeapTimer::SwapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void HeapTimer::SiftUp(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2; // father
    while (j >= 0)
    {
        if (heap_[j] < heap_[i]) // smallest at top
            break;
        SwapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::SiftDown(size_t index, size_t n)
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1; // left child
    while (j < n)
    {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) // j point to smaller child
            j++;

        if (heap_[i] < heap_[j])
            break;

        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index; // true means siftdown happened
}

void HeapTimer::Add(int id, unsigned long long timeout_ms, const TimeoutCallBack &cb, void *arg)
{
    assert(id >= 0);
    size_t i;
    if (ref_.count(id) == 0) // new node
    {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, timeout_ms, 0, cb, arg});
        SiftUp(i);
    }
    else
    {
        i = ref_[id];
        heap_[i].expires = timeout_ms;
        heap_[i].cb = cb;
        if (!SiftDown(i, heap_.size()))
        {
            SiftUp(i);
        }
    }
}

void HeapTimer::Del(size_t index)
{
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    size_t i = index;
    size_t n = heap_.size() - 1;
    if (i < n)
    {
        SwapNode(i, n);
        if (!SiftDown(i, n))
        {
            SiftUp(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::DoWork(int id)
{
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb(node.arg);
    Del(i);
}

void HeapTimer::Adjust(int id, unsigned long long new_timeout_ms)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = new_timeout_ms;
    SiftDown(ref_[id], heap_.size());
}

void HeapTimer::Again(int id, unsigned long long new_timeout_ms)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].newExpires = new_timeout_ms;
}

void HeapTimer::Pop()
{
    assert(!heap_.empty());
    Del(0);
}

void HeapTimer::Tick()
{
    // callback expired node
    while (!heap_.empty())
    {
        TimerNode *node = &heap_.front();
        if ((long long)(node->expires - getTickMs()) > 0)
        {
            break;
        }

        node->cb(node->arg);

        if (node->newExpires == 0)
        {
            Pop();
        }
        else
        {
            Adjust(node->id, node->newExpires);
            node->newExpires = 0;
        }
    }
}

void HeapTimer::Clear()
{
    ref_.clear();
    heap_.clear();
}

unsigned long long HeapTimer::GetNextTick()
{
    Tick();
    unsigned long long res = -1;
    if (!heap_.empty())
    {
        res = heap_.front().expires;
    }
    return res;
}