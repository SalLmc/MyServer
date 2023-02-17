#include "../http/http.h"

#include "memory_manage.hpp"

std::unordered_map<std::string, Type> typeMap{{typeid(void *).name(), Type::PV}, {typeid(Request *).name(), Type::P4REQUEST}};

HeapMemory heap;