file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

add_executable(RayTracer ${SOURCE_FILES})

target_compile_options(RayTracer PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    $<$<CONFIG:Release>:-O3>
    -Wall
)

target_compile_definitions(RayTracer PRIVATE
    MULTI_THREAD
)

target_include_directories(RayTracer PRIVATE
    ${PROJECT_SOURCE_DIR}/vendor
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(RayTracer PRIVATE
    Walnut
    $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    $<$<CXX_COMPILER_ID:GNU>:stdc++exp>
)

if (WIN32)
    target_compile_definitions(RayTracer PUBLIC WL_PLATFORM_WINDOWS)
endif()
