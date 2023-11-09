#include "../src/buffer/buffer.h"
#include "../src/headers.h"
#include "../src/log/logger.h"
#include "../src/utils/utils_declaration.h"

int main()
{
    std::unordered_map<std::string, std::string> mp;
    
    std::ifstream f("../types.json");
    nlohmann::json types = nlohmann::json::parse(f);
    mp = types.get<std::unordered_map<std::string, std::string>>();

    for (auto x : mp)
    {
        std::cout << x.first << "\t" << x.second << std::endl;
    }
}