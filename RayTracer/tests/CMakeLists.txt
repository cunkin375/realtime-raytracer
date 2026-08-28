file(GLOB_RECURSE SOURCE_FILES  ./*.cpp)

# Matrix.hpp uses <=> returning bool, which MSVC rejects (C7634).
# Exclude Matrix4Tests until Matrix.hpp is fixed.
if (MSVC)
    list(FILTER SOURCE_FILES EXCLUDE REGEX "Matrix4Tests\\.cpp$")
endif()

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
    $<$<CXX_COMPILER_ID:GNU>:stdc++exp>
)

gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)
