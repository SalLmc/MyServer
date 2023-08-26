#include "src/headers.h"

using namespace std;

int main()
{
    std::ifstream f("config.json");
    nlohmann::json data = nlohmann::json::parse(f);
    cout<<data["servers"]["port"]<<endl;
}