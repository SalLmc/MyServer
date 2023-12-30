#include "../src/headers.h"

int main()
{
    std::unordered_map<std::string, std::string> a, b;
    a = {{"1", "2"}};
    b = {{"1", "3"}, {"3", "4"}};

    for (auto &x : b)
    {
        a.insert(x);
    }

    for (auto &x : a)
    {
        std::cout << x.first << " " << x.second;
    }
}