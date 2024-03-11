#include <stdio.h>
#include <string>

int main()
{
    std::string a = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~!*'();:@&=+$,/?#[]";
    for(auto x:a)
    {
        printf("\'%c\',",x);
    }
}