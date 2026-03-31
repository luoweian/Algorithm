include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(gec_diff_test
    test_diff_value.cpp
    test_gec_diff.cpp
)

target_link_libraries(gec_diff_test
    gec_diff
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(gec_diff_test)
