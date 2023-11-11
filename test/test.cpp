#include "../src/buffer/buffer.h"
#include "../src/headers.h"
#include "../src/log/logger.h"
#include "../src/utils/utils_declaration.h"

int main()
{
    int a[]={1,2};
    int *x=a;
    int y=*x++;
    printf("%d\n",y);
}