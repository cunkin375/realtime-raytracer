cmake_minimum_required(VERSION 4.1.0)
project(Walnut)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Dependencies
# glfw handles its own setup. It builds a static library called 'glfw' by default
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/GLFW)

# imgui (using the CMakeLists.txt we just created)
add_subdirectory(vendor/imgui)

# Interface libraries for headers
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE vendor/glm)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE vendor/stb_image)

# Walnut Library
file(GLOB_RECURSE SRC_FILES Walnut/src/*.cpp Walnut/src/*.h)
add_library(Walnut STATIC ${SRC_FILES})

target_include_directories(Walnut PUBLIC
    Walnut/src
    vendor/imgui
    vendor/GLFW/include
    vendor/stb_image
    vendor/glm
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Walnut PUBLIC
    Vulkan::Vulkan
    glfw
    imgui
    glm
    stb_image
)

if (WIN32)
    target_compile_definitions(Walnut PUBLIC WL_PLATFORM_WINDOWS)
    target_link_libraries(Walnut PUBLIC Dwmapi)
endif()
