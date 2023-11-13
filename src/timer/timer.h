#ifndef TIMER_H
#define TIMER_H

#include "../headers.h"

struct TimerNode
{
    int id_;
    unsigned long long expires_;
    unsigned long long newExpires_=0;
    std::function<int(void *)> callback_;
    void *arg_;
    bool operator<(const TimerNode &t)
    {
        return expires_ < t.expires_;
    }
};

class HeapTimer
{
  public:
    HeapTimer()
    {
        heap_.reserve(64);
    }
    ~HeapTimer()
    {
        Clear();
    }
    void Adjust(int id, unsigned long long newExpireMs);
    void Again(int id, unsigned long long newExpireMs);
    void Add(int id, unsigned long long expireTimestampMs, const std::function<int(void *)> &cb, void *arg);
    void Remove(int id); // remove node 
    void DoWork(int id); // delete node && trigger its callback
    void Clear();
    // remove invalid nodes and call callbacks
    void Tick();
    void Pop();
    // -1ULL for empty timer, otherwise return the closest timestamp
    unsigned long long GetNextTick();

  private:
    void Del(size_t i);
    void SiftUp(size_t i);
    bool SiftDown(size_t index, size_t n);
    void SwapNode(size_t i, size_t j);
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif