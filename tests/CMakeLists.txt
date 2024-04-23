cmake_minimum_required(VERSION 3.14)
include(GoogleTest)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
find_package(sobjectizer REQUIRED CONFIG)

add_executable(evt_trans_tests test_event_basic.cpp)
target_link_libraries(evt_trans_tests event_transformer GTest::gtest_main
                      GTest::gmock_main sobjectizer::StaticLib)
gtest_discover_tests(evt_trans_tests)
