cmake_minimum_required(VERSION 3.6)
project(tron)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -pedantic -Wall")

set(SOURCE_FILES
    Makefile
        tron.c writing.c)

add_executable(tron ${SOURCE_FILES})