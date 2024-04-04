#include <stdio.h>
#include <string>

int main()
{
    printf("curl localhost:8081/");
    for(int i=0;i<4096;i++)
    {
        printf("x");
    }
    printf("\n");
}