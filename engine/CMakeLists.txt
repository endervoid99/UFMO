cmake_minimum_required(VERSION 3.29.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API  "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_STANDARD 20)
#add_compile_definitions(VK_USE_64_BIT_PTR_DEFINES=1)

project(engine VERSION 0.1.0 LANGUAGES C CXX)

#cmake_policy(SET CMP0146 OLD)
#cmake_policy(SET CMP0153 NEW)

#add_subdirectory(libs/eigen-3.4.0)
#add_subdirectory(libs)
add_subdirectory(libs/spdlog)

include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG        release-2.30.2
    GIT_PROGRESS    TRUE
    #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)
FetchContent_MakeAvailable(SDL2)

include(FetchContent)
FetchContent_Declare(
  Eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
# note: To disable eigen tests,
# you should put this code in a add_subdirectory to avoid to change
# BUILD_TESTING for your own project too since variables are directory
# scoped
set(BUILD_TESTING OFF)
set(EIGEN_BUILD_TESTING OFF)
set(EIGEN_MPL2_ONLY ON)
set(EIGEN_BUILD_PKGCONFIG OFF)
set(EIGEN_BUILD_DOC OFF)
FetchContent_MakeAvailable(Eigen)

include(FetchContent)
FetchContent_Declare(
    VMA
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        v3.0.1
    GIT_PROGRESS    TRUE
    #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)

FetchContent_MakeAvailable(VMA)

include(FetchContent)
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt
    GIT_TAG        v3.13.2
    GIT_PROGRESS    TRUE
    #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)

FetchContent_MakeAvailable(EnTT)

include(FetchContent)
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v0.5.2 # <HASH or TAG>
)
FetchContent_MakeAvailable(cpptrace)

FetchContent_Declare (
tracy
GIT_REPOSITORY https://github.com/wolfpld/tracy.git
GIT_TAG v0.10 #master
GIT_SHALLOW TRUE
GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable (tracy)

#FetchContent_Declare (
#ImGui
#GIT_REPOSITORY https://github.com/ocornut/imgui
#GIT_TAG v1.90.5 #master
#GIT_SHALLOW TRUE
#GIT_PROGRESS TRUE
#)
#FetchContent_MakeAvailable (ImGui)

find_package(Vulkan REQUIRED )

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.5
)
FetchContent_GetProperties(imgui)
#if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/)
    file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
    file(GLOB IMGUI_HEADERS ${imgui_SOURCE_DIR}/*.h)
    add_library(imgui ${IMGUI_SOURCES} ${IMGUI_SOURCES} ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp )
    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
    target_link_libraries(imgui SDL2main SDL2-static Vulkan::Vulkan)
    message(STATUS "----------------------------------------")
    message(STATUS "----------------------------------------")
    message(STATUS ${IMGUI_SOURCES})
    message(STATUS "----------------------------------------")
    message(STATUS "----------------------------------------")
     #add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

    #find_package(imgui CONFIG REQUIRED)
 
#find_package(VMA CONFIG REQUIRED)
#find_package(Eigen3 REQUIRED ) 


#add_library(engine include/engine/hello.h src/defines.h src/hello.cpp)
add_library(engine "")
include(sourcelist.cmake)

option(TRACY_ENABLE ON)
target_link_libraries(${PROJECT_NAME} TracyClient)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan VulkanMemoryAllocator Eigen3::Eigen spdlog::spdlog_header_only  SDL2main SDL2-static EnTT::EnTT cpptrace::cpptrace imgui)


target_include_directories(${PROJECT_NAME} 
    PUBLIC ${PROJECT_SOURCE_DIR}/include ${IMGUI_INCLUDE_DIR})

# On windows copy cpptrace.dll to the same directory as the executable for your_target
if(WIN32)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:cpptrace::cpptrace>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()    

