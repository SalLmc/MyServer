#include "http.h"

Header::Header(std::string &&namee, std::string &&valuee)
{
    name = namee;
    value = valuee;
}
