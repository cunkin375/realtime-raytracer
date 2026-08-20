cmake_minimum_required(VERSION 4.1.0)
project(RayTracer)

option(ENABLE_TESTS "Compile tests" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(VENDOR_DIR ${PROJECT_SOURCE_DIR}/vendor)

add_subdirectory(src)

# MSVC fails to compile testing directory
if (ENABLE_TESTS)
    include(FetchContent)
    set(FETCHCONTENT_QUIET FALSE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        SOURCE_DIR ${VENDOR_DIR}/googletest
    )

    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()
