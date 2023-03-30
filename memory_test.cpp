// #include "src/core/core.h"
// #include "src/http/http.h"

// #include "src/memory/memory_manage.hpp"

// extern HeapMemory heap;

#include "src/headers.h"

class Test
{
  public:
    Test(std::string &&str) : val(str)
    {
    }
    Test &operator=(const Test &) = delete;
    // Test(const Test &) = delete;
    // Test(Test &other):val(other.val)
    // {
    //     printf("copy ctor without const\n");
    // }
    // Test(Test &&other) : val(std::move(other.val))
    // {
    //     printf("move ctor\n");
    // }
    std::string val;
};

int main()
{
    const Test t("SDF");

    Test tt(t);

    printf("%s\n", tt.val.data());

    // Test *a=heap.hNew<Test>(std::move(t));

    // printf("%s\n",t.val.data());
    // printf("%s\n",a->val.data());
}