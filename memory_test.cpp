#include "src/core/core.h"
#include "src/http/http.h"

#include "src/core/memory_manage.hpp"

extern HeapMemory heap;

int main()
{
    auto r = heap.hNew<Request>();
    r->idx_ = 2;
    printf("r:%d\n", r->idx_);
    int *a = (int *)heap.hMalloc(100);
}