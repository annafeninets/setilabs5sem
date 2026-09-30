#include "network_interface.h"

#include <net/if.h>
#include <stdexcept>
#include <string>

namespace setilab1
{
    unsigned int resolveInterfaceIndex(const char *name)
    {
        unsigned int index = if_nametoindex(name);
        if (index == 0)
            throw std::runtime_error(std::string("Сетевой интерфейс не найден: ") + name);
        return index;
    }
}
