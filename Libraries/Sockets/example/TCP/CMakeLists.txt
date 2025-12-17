cmake_minimum_required(VERSION 3.16)
project(tcp)
set(CMAKE_CXX_STANDARD 17)

add_executable(tcp_server tcp_server.cpp)

add_executable(tcp_client tcp_client.cpp)