cmake_minimum_required(VERSION 3.16)
project(lab3_places CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
  FetchContent_MakeAvailable(json)
endif()

add_executable(lab3_places
  src/main.cpp
  src/config.cpp
  src/url_builder.cpp
  src/util.cpp
  src/http_client.cpp
  src/api_client.cpp
  src/providers.cpp
  src/place_service.cpp
  src/text_report_renderer.cpp
  src/console_app.cpp
        src/retrying_http_client.cpp
        src/place_kinds.hpp
        src/place_kinds.cpp
        src/retrying_http_client.hpp)
target_link_libraries(lab3_places PRIVATE CURL::libcurl nlohmann_json::nlohmann_json Threads::Threads)
if(MSVC)
  target_compile_options(lab3_places PRIVATE /utf-8 /W4)   # исходники в UTF-8 (русский текст)
else()
  target_compile_options(lab3_places PRIVATE -Wall -Wextra)
endif()

# .env кладём рядом с исполняемым файлом (удобно запускать из CLion и из терминала)
if(EXISTS ${CMAKE_SOURCE_DIR}/.env)
  add_custom_command(TARGET lab3_places POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/.env $<TARGET_FILE_DIR:lab3_places>/.env)
endif()
