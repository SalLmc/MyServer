#include "../headers.h"

#include "../utils/utils_declaration.h"
#include "timer.h"

TimerNode::TimerNode(int id, std::string name, unsigned long long expires, unsigned long long newExpires,
                     std::function<int(void *)> callback, void *arg)
    : id_(id), name_(name), expires_(expires), newExpires_(newExpires), callback_(callback), arg_(arg)
{
}

HeapTimer::HeapTimer()
{
    heap_.reserve(64);
}

HeapTimer::~HeapTimer()
{
    clear();
}

void HeapTimer::swapNode(size_t i, size_t j)
{
    std::swap(heap_[i], heap_[j]);
    idIndexMap[heap_[i].id_] = i;
    idIndexMap[heap_[j].id_] = j;
}

void HeapTimer::siftUp(size_t i)
{
    size_t j = (i - 1) / 2; // father
    while (j >= 0 && j < heap_.size())
    {
        if (heap_[j] < heap_[i]) // smallest at top
            break;
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::siftDown(size_t index, size_t n)
{
    size_t i = index;
    size_t j = i * 2 + 1; // left child
    while (j < n)
    {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) // j point to smaller child
            j++;

        if (heap_[i] < heap_[j])
            break;

        swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index; // true means siftdown happened
}

void HeapTimer::add(int id, std::string name, unsigned long long expireTimestampMs,
                    const std::function<int(void *)> &cb, void *arg)
{
    size_t i;
    if (idIndexMap.count(id) == 0) // new node
    {
        i = heap_.size();
        idIndexMap[id] = i;
        heap_.emplace_back(id, name, expireTimestampMs, 0, cb, arg);
        siftUp(i);
    }
    else
    {
        i = idIndexMap[id];
        heap_[i].expires_ = expireTimestampMs;
        heap_[i].callback_ = cb;
        if (!siftDown(i, heap_.size()))
        {
            siftUp(i);
        }
    }
}

void HeapTimer::del(size_t index)
{
    size_t i = index;
    size_t n = heap_.size() - 1;
    if (i < n)
    {
        swapNode(i, n);
        if (!siftDown(i, n))
        {
            siftUp(i);
        }
    }
    idIndexMap.erase(heap_.back().id_);
    heap_.pop_back();
}

void HeapTimer::remove(int id)
{
    if (heap_.empty() || idIndexMap.count(id) == 0)
    {
        return;
    }
    del(idIndexMap[id]);
}

void HeapTimer::handle(int id)
{
    if (heap_.empty() || idIndexMap.count(id) == 0)
    {
        return;
    }
    size_t i = idIndexMap[id];
    TimerNode node = heap_[i];
    node.callback_(node.arg_);
    del(i);
}

void HeapTimer::adjust(int id, unsigned long long new_timeout_ms)
{
    size_t i = idIndexMap[id];
    size_t n = heap_.size();
    heap_[i].expires_ = new_timeout_ms;
    if (!siftDown(i, n))
    {
        siftUp(i);
    }
}

void HeapTimer::again(int id, unsigned long long newExpireMs)
{
    heap_[idIndexMap[id]].newExpires_ = newExpireMs;
}

void HeapTimer::pop()
{
    del(0);
}

void HeapTimer::tick()
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
            pop();
        }
        else
        {
            adjust(node->id_, node->newExpires_);
            node->newExpires_ = 0;
        }
    }
}

void HeapTimer::clear()
{
    idIndexMap.clear();
    heap_.clear();
}

unsigned long long HeapTimer::getNextTick()
{
    unsigned long long res = -1;
    if (!heap_.empty())
    {
        res = heap_.front().expires_;
    }
    return res;
}

void HeapTimer::printActiveTimer()
{
    for (auto &x : heap_)
    {
        printf("id: %d, name:%s\n", x.id_, x.name_.c_str());
    }
}