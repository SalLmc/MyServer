#include "src/core/core.h"
#include "src/http/http.h"

#include "src/memory/memory_manage.hpp"

extern HeapMemory heap;

int main()
{
    auto r = heap.hNew<Request>();
}