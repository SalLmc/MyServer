#include <stdio.h>
#include <unordered_map>

#define HELLOHTML                                                                                                      \
    "HTTP/1.1 200 OK\r\n"                                                                                              \
    "Content-Length: 21\r\n"                                                                                           \
    "Content-Type: text/html\r\n\r\n"                                                                                  \
    "<em>Hello World</em>\r\n"

int main()
{
    size_t i,j;
    i=0;
    j=(i-1)/2;
    printf("%lld",j==0);
}