file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not detected, if installed set
                     VULKAN_SDK env variable or manually provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

include_directories(SYSTEM $ENV{VULKAN_SDK}/include)
find_package(Vulkan REQUIRED COMPONENTS dxc)

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
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(RayTracer PRIVATE
    Walnut
    Vulkan::Vulkan
    Vulkan::dxc_lib
    $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    $<$<CXX_COMPILER_ID:GNU>:stdc++exp>
)



set_target_properties(RayTracer PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH $ENV{VULKAN_SDK}/lib
)

if (WIN32)
    target_compile_definitions(RayTracer PUBLIC WL_PLATFORM_WINDOWS)
endif()

option(COMPILE_SHADERS "Compile Shaders" OFF)

# Compile Shaders to SPIR-V
if (COMPILE_SHADERS)
    find_program(DXC_EXECUTABLE dxc HINTS "$ENV{VULKAN_SDK}/bin")

    set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../assets/shaders/raytracer.hlsl")
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/raytracer.spv")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${DXC_EXECUTABLE}
            -spirv                        # compile for vulkan
            -T cs_6_5                     # target version
            -E main                       # entry point
            -fspv-target-env="vulkan1.3"  # vulkan environment
            ${SHADER_SOURCE}
            -Fo ${SHADER_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling HLSL to SPIR-V using DXC"
    )

    target_sources(RayTracer PRIVATE ${SHADER_OUTPUT})
endif()
