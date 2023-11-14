#ifndef TIMER_H
#define TIMER_H

#include "../headers.h"

class TimerNode
{
  public:
    TimerNode() = delete;
    TimerNode(int id, std::string name, unsigned long long expires, unsigned long long newExpires,
              std::function<int(void *)> callback, void *arg);
    int id_;
    std::string name_;
    unsigned long long expires_;
    unsigned long long newExpires_;
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
    HeapTimer();
    ~HeapTimer();

    void adjust(int id, unsigned long long newExpireMs);
    void again(int id, unsigned long long newExpireMs);
    void add(int id, std::string name, unsigned long long expireTimestampMs, const std::function<int(void *)> &cb, void *arg);
    void remove(int id); // remove node
    void handle(int id); // delete node && trigger its callback
    void clear();
    // remove invalid nodes and call callbacks
    void tick();
    void pop();
    // -1ULL for empty timer, otherwise return the closest timestamp
    unsigned long long getNextTick();
    void printActiveTimer();

  private:
    void del(size_t i);
    void siftUp(size_t i);
    bool siftDown(size_t index, size_t n);
    void swapNode(size_t i, size_t j);
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> idIndexMap;
};

#endif