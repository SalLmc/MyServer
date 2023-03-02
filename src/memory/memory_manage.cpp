#include "../http/http.h"

#include "memory_manage.hpp"

std::unordered_map<std::type_index, Type> typeMap{{std::type_index(typeid(void *)), Type::PV},
                                                  {std::type_index(typeid(Request *)), Type::P4REQUEST}};

HeapMemory heap;