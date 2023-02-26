#include <stdio.h>
#include <unordered_map>
#include <sys/file.h>
#include <unistd.h>

#define HELLOHTML                                                                                                      \
    "HTTP/1.1 200 OK\r\n"                                                                                              \
    "Content-Length: 21\r\n"                                                                                           \
    "Content-Type: text/html\r\n\r\n"                                                                                  \
    "<em>Hello World</em>\r\n"

int main()
{
    int fd=open("makefile",O_RDONLY);
    close(fd);
    close(fd);
}