cmake_minimum_required(VERSION 3.15)
project(raylib_app C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -march=native")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

set(SOURCES
    src/main.c
)
add_executable(raylib_app ${SOURCES})

# Use a system-installed raylib if present; otherwise download and build it.
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    message(STATUS "raylib not found on system, fetching it")
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

target_link_libraries(raylib_app raylib)

if(APPLE)
    target_link_libraries(raylib_app "-framework CoreVideo" "-framework IOKit" "-framework Cocoa" "-framework GLUT" "-framework OpenGL")
endif()
