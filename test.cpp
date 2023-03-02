#include <assert.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <unordered_map>
#include "src/util/utils_declaration.h"

int main()
{
    std::string a = "sdf/sdf+/ /sdf";

    std::string b=UrlEncode(a,'/');
    printf("%s\n", b.c_str());
    printf("%s\n",UrlDecode(b).c_str());
}