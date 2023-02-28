#include "src/core/core.h"
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <unordered_map>

#define HELLOHTML                                                                                                      \
    "HTTP/1.1 200 OK\r\n"                                                                                              \
    "Content-Length: 21\r\n"                                                                                           \
    "Content-Type: text/html\r\n\r\n"                                                                                  \
    "<em>Hello World</em>\r\n"

int main()
{
    Dir dir(opendir("src"));

    while (dir.readDir() == OK)
    {
        dir.getStat();
        printf("d_name= %s\n", dir.de->d_name);
        printf("d_type= %d\n", dir.de->d_type);
        printf("%ld\n", dir.info.st_size / 1024 / 1024);

        char tmp[100];
        memset(tmp, 0, sizeof(tmp));
        tm t;
        memset(&t,0,sizeof(tm));
        gmtime_r(&dir.info.st_mtim.tv_sec, &t);
        t.tm_gmtoff = 8 * 3600;
        strftime(tmp, 99, "%Y-%m-%d %H:%M:%S.", &t);
        printf("%s\n", tmp);
    }
}