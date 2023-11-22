#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/utils/my_json.h"

int main()
{
    char buffer[1024] = {0};
    Fd fd = open("../config.json", O_RDONLY);

    read(fd.getFd(), buffer, 1023);

    std::vector<Token> tokens(1024);

    JsonParser parser(&tokens, buffer, 1024);
    auto json = parser.parse();

    Token *server = json.get("servers").get(1).value();

    Token *tmp = json.get("servers").get(1).get("auth_path").get(0).value();

    printf("%s\n", server->toString(buffer).c_str());
    printf("%s\n", tmp->toString(buffer).c_str());
}