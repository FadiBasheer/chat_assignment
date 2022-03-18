cmake_minimum_required(VERSION 3.21)
project(chat_assignment C)

set(CMAKE_C_STANDARD 11)

add_executable(client client.c)
add_executable(servre server.c)

add_executable(servre2 server2.c)

