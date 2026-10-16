cmake_minimum_required(VERSION 3.10)
project(tcp_file_transfer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_library(common STATIC
    common/tcp_connection.cpp
    common/platform.cpp
    common/utf8.cpp
)

if(WIN32)
    target_link_libraries(common PUBLIC ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(common PUBLIC Threads::Threads)
endif()

add_executable(client
    client/main.cpp
    client/file_client.cpp
)
target_link_libraries(client PRIVATE common)

add_executable(server
    server/main.cpp
    server/server.cpp
    server/client_session.cpp
    server/speed_tracker.cpp
)
target_link_libraries(server PRIVATE common)
