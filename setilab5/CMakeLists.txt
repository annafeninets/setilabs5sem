cmake_minimum_required(VERSION 3.10)
project(socks5_proxy CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(socks5_proxy main.cpp server.cpp session.cpp resolver.cpp epoll.cpp socks5.cpp dns.cpp
        dns.hpp
        epoll.hpp
        fd.hpp
        session.hpp
        socks5.hpp
        session.hpp
        buffer.hpp
        resolver.hpp
        server.hpp)
target_compile_options(socks5_proxy PRIVATE -Wall -Wextra -O2)
