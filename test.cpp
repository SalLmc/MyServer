#include <stdio.h>

class node
{
    int x = 1;

  public:
    ~node()
    {
        x = -1;
        printf("destructing\n");
    }
};

int main()
{
    node a;
    node &b=a;
    
}