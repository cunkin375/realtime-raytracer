file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(unit_tests ${SOURCE_FILES})

target_include_directories(unit_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/vendor
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
    Walnut
)

gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)
