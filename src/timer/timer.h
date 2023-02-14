#ifndef TIMER_H
#define TIMER_H

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <functional>
#include <queue>
#include <time.h>
#include <unordered_map>

typedef std::function<int(void *)> TimeoutCallBack;
typedef unsigned long long TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeStamp newExpires=0;
    TimeoutCallBack cb;
    void *arg;
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
    void Add(int id, unsigned long long timeout_ms, const TimeoutCallBack &cb, void *arg);
    void DoWork(int id); // delete node id && trigger id's callback
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