include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(diff_lib_test
    test_diff_value.cpp
    test_diff_client.cpp
)

target_link_libraries(diff_lib_test
    diff_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(diff_lib_test)
