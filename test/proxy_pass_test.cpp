#include "../src/core/core.h"
#include "../src/headers.h"
#include "../src/utils/utils_declaration.h"
int main()
{
    Logger logger("log/", "test", 1);

    std::string from = "/webapp/";
    std::string to = "http://localhost:x/sdfsdf/dfg";
    std::string request_uri = "/webapp/foo?bar=baz";
    try
    {
        auto x = getServer(to);

        std::cout << x.first << "\n" << x.second << "\n";

        // replace uri
        std::string newUri = getLeftUri(to) + request_uri.replace(0, from.length(), "");

        std::cout << newUri << "\n";
    }
    catch (const std::exception &e)
    {
        __LOG_INFO_INNER(logger) << e.what();
    }
}