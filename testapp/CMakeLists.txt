cmake_minimum_required(VERSION 3.5.0)
project(testapp VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

add_executable(testapp main.cpp)

target_link_libraries(testapp
    PRIVATE engine)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)   

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)
message(STATUS ${PROJECT_SOURCE_DIR})
message(STATUS ${GLSL_SOURCE_FILES})

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
 
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

