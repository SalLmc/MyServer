#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

int main(int argc, char *argv[])
{
    char a[5]={'1','2','3','4','5'};
    char b[]="i am str b";

    int x=strlen(a);
    int y=strlen(b);
    printf("%s %d\n",a,x);
}