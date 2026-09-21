cmake_minimum_required(VERSION 3.16)

project(cpp_web_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server server.cpp)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(server PRIVATE pthread)