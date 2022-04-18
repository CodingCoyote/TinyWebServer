cmake_minimum_required(VERSION 2.8)

project(server)

find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(/usr/include/mariadb /usr/include/mariadb/mysql)

add_executable(server src/main.cpp src/http/httpconn.cpp src/sqlConn/sqlConn.cpp)

target_link_libraries(server nlohmann_json::nlohmann_json /usr/lib/x86_64-linux-gnu/libmysqlclient.so)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lmysqlclient -ldl -lm -lssl -lcrypto")

