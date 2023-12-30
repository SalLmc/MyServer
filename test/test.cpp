#include "../src/headers.h"

int main()
{
    switch (fork())
    {
    case 0:
        while (1)
        {
            printf("i am child");
            sleep(1);
        }
        break;

    default:
        while (1)
            ;
        break;
    }
}