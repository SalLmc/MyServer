#include "../http/http.h"

#include "memory_manage.hpp"

std::unordered_map<std::type_index, Type> typeMap{{std::type_index(typeid(void *)), Type::VOID},
                                                  {std::type_index(typeid(Request *)), Type::REQUEST},
                                                  {std::type_index(typeid(Upstream *)), Type::UPSTREAM}};

HeapMemory heap;