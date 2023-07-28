#include "src/headers.h"

#include "src/util/utils_declaration.h"

int main()
{
    int fd = open("pid_file", O_RDWR);
    std::string s=fd2Path(fd);
    if(s!="")
    {
        printf("%s\n",s.data());
    }
    std::string z="";
    printf("%d\n",z.empty());
}