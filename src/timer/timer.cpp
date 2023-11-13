#include "../headers.h"

#include "timer.h"
#include "../utils/utils_declaration.h"

void HeapTimer::SwapNode(size_t i, size_t j)
{
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id_] = i;
    ref_[heap_[j].id_] = j;
}

void HeapTimer::SiftUp(size_t i)
{
    size_t j = (i - 1) / 2; // father
    while (j >= 0 && j < heap_.size())
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

void HeapTimer::Add(int id, unsigned long long timeoutstamp_ms, const std::function<int(void *)> &cb, void *arg)
{
    size_t i;
    if (ref_.count(id) == 0) // new node
    {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, timeoutstamp_ms, 0, cb, arg});
        SiftUp(i);
    }
    else
    {
        i = ref_[id];
        heap_[i].expires_ = timeoutstamp_ms;
        heap_[i].callback_ = cb;
        if (!SiftDown(i, heap_.size()))
        {
            SiftUp(i);
        }
    }
}

void HeapTimer::Del(size_t index)
{
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
    ref_.erase(heap_.back().id_);
    heap_.pop_back();
}

void HeapTimer::Remove(int id)
{
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    Del(ref_[id]);
}

void HeapTimer::DoWork(int id)
{
    if (heap_.empty() || ref_.count(id) == 0)
    {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.callback_(node.arg_);
    Del(i);
}

void HeapTimer::Adjust(int id, unsigned long long new_timeout_ms)
{
    size_t i = ref_[id];
    size_t n = heap_.size();
    heap_[i].expires_ = new_timeout_ms;
    if (!SiftDown(i, n))
    {
        SiftUp(i);
    }
}

void HeapTimer::Again(int id, unsigned long long new_timeout_ms)
{
    heap_[ref_[id]].newExpires_ = new_timeout_ms;
}

void HeapTimer::Pop()
{
    Del(0);
}

void HeapTimer::Tick()
{
    // callback expired node
    while (!heap_.empty())
    {
        TimerNode *node = &heap_.front();
        if ((long long)(node->expires_ - getTickMs()) > 0)
        {
            break;
        }

        node->callback_(node->arg_);

        if (node->newExpires_ == 0)
        {
            Pop();
        }
        else
        {
            Adjust(node->id_, node->newExpires_);
            node->newExpires_ = 0;
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
    unsigned long long res = -1;
    if (!heap_.empty())
    {
        res = heap_.front().expires_;
    }
    return res;
}