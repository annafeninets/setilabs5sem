#include "Application.h"
#include "Multicast address.h"

#include <exception>
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Использование: ./lab1 <multicast-адрес>\n";
        return 1;
    }

    try
    {
        MulticastAddress group(argv[1]);
        Application app(group);
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}