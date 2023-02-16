#ifndef MEMORY_MANAGE_H
#define MEMORY_MANAGE_H

#include <list>
#include <string>
#include <typeinfo>
#include <unordered_map>

enum class Type
{
    PV,
    P4REQUEST
};

extern std::unordered_map<std::string, Type> typeMap;

class AnyPtr
{
  public:
    AnyPtr() = delete;
    template <typename T> AnyPtr(T *t) : type(typeid(t).name()), addr(static_cast<void *>(t))
    {
    }
    bool operator==(const AnyPtr &a)
    {
        return a.addr == addr;
    }
    std::string type;
    void *addr;
};

class HeapMemory
{
  public:
    HeapMemory() = default;
    ~HeapMemory()
    {
        while (!ptrs_.empty())
        {
            AnyPtr &front = ptrs_.front();
            if (typeMap.count(front.type))
            {
                switch (typeMap[front.type])
                {
                case Type::P4REQUEST:
                    delete (Request *)front.addr;
                    break;

                default:
                    free(front.addr);
                    break;
                }
            }
            ptrs_.pop_front();
        }
    }

    void *hMalloc(size_t size)
    {
        void *ptr = malloc(size);
        ptrs_.emplace_back(ptr);
        return ptr;
    }

    void hFree(void *ptr)
    {
        free(ptr);
        ptrs_.remove(AnyPtr(ptr));
    }
    template <typename T, typename... Args> T *hNew(Args... args)
    {
        T *ptr = new T(args...);
        ptrs_.emplace_back(ptr);
        return ptr;
    }
    template <typename T> void hDelete(T *ptr)
    {
        delete ptr;
        ptrs_.remove(AnyPtr(ptr));
    }

  private:
    std::list<AnyPtr> ptrs_;
};

#endif