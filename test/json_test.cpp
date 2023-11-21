#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/utils/my_json.h"

int main()
{
    char buffer[1024] = {0};
    Fd fd = open("../config.json", O_RDONLY);

    read(fd.getFd(), buffer, 1023);

    JsonParser parser(buffer, 1024);
    parser.doParse();

    Token *servers = parser.get("servers", &parser.tokens_[0]);
    Token *sec = parser.get(1, servers);

    printf("%s\n", sec->toString(buffer).c_str());

    while (1)
        ;
}