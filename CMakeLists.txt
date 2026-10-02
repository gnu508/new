cmake_minimum_required(VERSION 3.20)
project(watermelon_impact LANGUAGES C CXX)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glfw glad)

find_package(OpenGL REQUIRED)
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

add_executable(watermelon_impact WIN32 src/main.cpp)
target_compile_features(watermelon_impact PRIVATE cxx_std_17)
target_link_libraries(watermelon_impact PRIVATE glfw glad_gl_core_33 OpenGL::GL)

if(MSVC)
    target_compile_options(watermelon_impact PRIVATE /W4)
else()
    target_compile_options(watermelon_impact PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    target_link_options(watermelon_impact PRIVATE -static)
endif()