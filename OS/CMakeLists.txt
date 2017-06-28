cmake_minimum_required(VERSION 3.3)
project(oslabpi)

include_directories(
        "include"
        "./"
        "drivers/include"
        "user"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")
add_executable(nouse dummy.c)