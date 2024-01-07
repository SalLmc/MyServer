#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/utils/utils.h"

Server *serverPtr;
bool enable_logger = 1;

int main()
{
    u_char lowcase[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                       "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0"
                       "0123456789\0\0\0\0\0\0"
                       "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0_"
                       "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
                       "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                       "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                       "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                       "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    for (int i = 0; i < sizeof(lowcase) / sizeof(u_char); i++)
    {
        printf("%d: %c\n", i, lowcase[i]);
    }
}