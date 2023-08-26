#include "src/headers.h"

using namespace std;

int main()
{
    std::ifstream f("config.json");
    nlohmann::json data = nlohmann::json::parse(f);
    nlohmann::json a=data["servers"][0];

    std::string from=a["proxy"];

    cout<<from<<endl;
}