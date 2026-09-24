cmake_minimum_required(VERSION 3.29)
project(setilab1)

set(CMAKE_CXX_STANDARD 20)

add_executable(setilab1
        src/main.cpp
        "include/Constants.h"
        "src/Application.cpp"
        "include/Application.h"
        "src/Peer tracker.cpp"
        "include/Peer tracker.h"
        "src/Multicast receiver.cpp"
        "include/Multicast receiver.h"
        "src/Multicast sender.cpp"
        "include/Multicast sender.h"
        "src/Multicast address.cpp"
        "include/Multicast address.h"
)