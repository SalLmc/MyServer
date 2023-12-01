#include "../src/headers.h"

#include "../src/core/core.h"
#include "../src/utils/json.hpp"

using std::cout;
using std::endl;
using std::string;

int main()
{
    MemFile file(open("../config.json", O_RDONLY));

    JsonParser parser(file.addr_, file.len_);
    auto json = parser.parse();

    auto tmp = json["daemon"].value<bool>();

    cout << tmp << endl;
}