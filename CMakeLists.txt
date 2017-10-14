cmake_minimum_required(VERSION 3.8)
project(deepB_remake)

add_definitions(-Wall -Werror -Wextra)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(deepB_remake ${SOURCE_FILES})