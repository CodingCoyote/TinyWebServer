cmake_minimum_required(VERSION 2.8)

project(server)

find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(server src/main.cpp src/http/httpconn.cpp)

target_link_libraries(server nlohmann_json::nlohmann_json)

#include_directories(include)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -pthread")


