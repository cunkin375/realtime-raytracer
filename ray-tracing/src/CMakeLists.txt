file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

# # Glad files
# set(GLAD_INCLUDE_DIR ${VENDOR_DIR}/glad/include)
# set(GLAD_SOURCE ${VENDOR_DIR}/glad/src/glad.c)
# # GLFW files
# set(GLFW_INCLUDE_DIR ${VENDOR_DIR}/GLFW/include)
# set(GLFW_LIBS ${VENDOR_DIR}/GLFW/lib)
# # Assimp setup
# # set(ASSIMP_INCLUDE_DIR ${VENDOR_DIR}/assimp/include)
# # Windows Libraries
# set(MINGW_LIBS ${GLFW_LIBS}/mingw-w64)
# # Linux-Compiled
# set(LINUX_LIBS ${GLFW_LIBS}/linux)

set(VENDOR_SOURCE ${GLAD_SOURCE})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${VENDOR_SOURCE})

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    -Wall
    -O3
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/src
    # ${GLFW_INCLUDE_DIR}
    # ${GLAD_INCLUDE_DIR}
    #    ${ASSIMP_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    # $<$<PLATFORM_ID:Windows>:${MINGW_LIBS}/libglfw3.a>
    # $<$<PLATFORM_ID:Linux>:${LINUX_LIBS}/libglfw3.a>
    # assimp
    # these MUST be the last arguments or else the linker will throw a fit
    $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    $<$<CXX_COMPILER_ID:GNU>:stdc++exp>
)

# GLFW dependencies for linux
# if(UNIX AND NOT APPLE)
#     target_link_libraries(${PROJECT_NAME} PRIVATE 
#         X11
#         pthread
#         dl
#     )
# endif()
