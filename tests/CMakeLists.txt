find_package(GTest REQUIRED)
add_executable(falcon_tests
  sample_test.cpp
)
target_link_libraries(falcon_tests PRIVATE falcon_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(falcon_tests)
