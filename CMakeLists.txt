cmake_minimum_required(VERSION 3.10)
project(receiver C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "-pthread")       # -fsanitize=address -g")

add_executable(receiver err.c err.h radio.c radio.h receiver.c receiver.h transmitter_handler.c transmitter_handler.h buffer_handler.c buffer_handler.h)