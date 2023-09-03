#ifndef TIMER_H
#define TIMER_H

#include "../headers.h"

struct TimerArgs
{
    TimerArgs(unsigned long long interval, int size);
    ~TimerArgs();
    unsigned long long interval;
    int size;
    void **args;
};

struct TimerNode
{
    int id;
    unsigned long long expires;
    unsigned long long newExpires = 0;
    std::function<int(TimerArgs)> cb;
    TimerArgs arg;
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
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
    void Adjust(int id, unsigned long long new_timeout_ms);
    void Again(int id, unsigned long long new_timeout_ms);
    void Add(int id, unsigned long long timeoutstamp_ms, const std::function<int(TimerArgs)> &cb, TimerArgs arg);
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