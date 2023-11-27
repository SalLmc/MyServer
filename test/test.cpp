#include "../src/utils/utils_declaration.h"

int main()
{
    auto t = getTickMs();
    printf("%lld\n", t);
    sleep(1);
    t = getTickMs();
    printf("%lld\n", t);
}