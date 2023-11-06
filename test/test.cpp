#include "../src/buffer/buffer.h"
#include "../src/headers.h"
#include "../src/log/logger.h"
#include "../src/utils/utils_declaration.h"

int main()
{
    LinkedBuffer buffer;
    std::string a = "0123456789";
    printf("%ld\n", a.size());
    for (int i = 0; i < 1024; i++)
    {
        buffer.append(a);
    }

    // 2048
    printf("%ld\n%ld\n", buffer.nodes.size(), buffer.nodes.back().readableBytes());
}