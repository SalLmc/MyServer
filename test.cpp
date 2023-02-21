#include <stdio.h>
#include <unordered_map>

#define HELLOHTML                                                                                                      \
    "HTTP/1.1 200 OK\r\n"                                                                                              \
    "Content-Length: 21\r\n"                                                                                           \
    "Content-Type: text/html\r\n\r\n"                                                                                  \
    "<em>Hello World</em>\r\n"

class node
{
  public:
    int val = 0;
    ~node()
    {
        printf("des\n");
    }
};

std::unordered_map<int, node> mp;

int main()
{
    node a;
    mp[0] = a;
    printf("%d\n", mp[0].val);
    a.val = 1;
    printf("%d\n", mp[0].val);
}