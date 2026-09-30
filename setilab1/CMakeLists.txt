cmake_minimum_required(VERSION 3.29)
project(setilab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(setilab1
        src/main.cpp
        src/application.cpp
        src/heartbeat.cpp
        src/instance_id.cpp
        src/multicast_address.cpp
        src/multicast_receiver.cpp
        src/multicast_sender.cpp
        src/network_interface.cpp
        src/peer_tracker.cpp
        src/udp_socket.cpp
        include/application.h
        include/constants.h
        include/heartbeat.h
        include/instance_id.h
        include/multicast_address.h
        include/multicast_receiver.h
        include/multicast_sender.h
        include/network_interface.h
        include/peer_tracker.h
        include/time_utils.h
        include/udp_socket.h
)

target_include_directories(setilab1 PRIVATE include)
target_compile_options(setilab1 PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)