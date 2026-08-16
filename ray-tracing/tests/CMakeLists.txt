file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

set(GLM_INCLUDE_DIR ${VENDOR_DIR}/glm/include)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(unit_tests ${SOURCE_FILES})

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    # glm
    ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
)

gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)
