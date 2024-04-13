cmake_minimum_required(VERSION 3.5.0)
project(engine VERSION 0.1.0 LANGUAGES C CXX)

cmake_policy(SET CMP0146 OLD)
cmake_policy(SET CMP0153 NEW)

add_subdirectory(libs/eigen-3.4.0)

find_package(Vulkan REQUIRED FATAL_ERROR) 
find_package(Eigen3 REQUIRED FATAL_ERROR) 

add_library(engine include/engine/hello.h src/defines.h src/hello.cpp)

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan Eigen3::Eigen)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include)

