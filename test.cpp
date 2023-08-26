#include "src/core/core.h"
#include "src/headers.h"

using namespace std;

int main()
{
    umask(0);
    int spre = umask(0);
    printf("%d\n", spre);
    Fd fd(open("testfile", O_RDWR | O_CREAT, 0666));
}