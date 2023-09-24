#include "src/headers.h"

using namespace std;

int main()
{
    int cores = sysconf(_SC_NPROCESSORS_CONF);
    printf("%d\n", cores);
}