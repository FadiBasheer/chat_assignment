cmake_minimum_required(VERSION 3.21)
project(for_delete C)

set(CMAKE_C_STANDARD 11)

add_executable(server servre.c)
add_executable(client client.c)

