#include "src/headers.h"

#include "src/core/core.h"

int main()
{
    ServerAttribute a(8081, "/home/sallmc/dist", "index.html", "/api/", "http://175.178.175.106:8080/", 0,{"index.html"});

    for(auto &x:a.try_files)
    {
        printf("%s\n",x.c_str());
    }
}