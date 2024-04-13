cmake_minimum_required(VERSION 3.5.0)
project(UFMO VERSION 0.1.0 LANGUAGES C CXX)

#include(CTest)
#enable_testing()

#add_executable(mytest main.cpp)
add_subdirectory(engine)
add_subdirectory(testapp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
