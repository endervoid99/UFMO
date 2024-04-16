cmake_minimum_required(VERSION 3.5.0)
project(testapp VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

add_executable(testapp main.cpp)

target_link_libraries(testapp
    PRIVATE engine)

