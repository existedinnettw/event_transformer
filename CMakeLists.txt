cmake_minimum_required(VERSION 3.15)
project(event_transformer CXX)

add_library(event_transformer INTERFACE)
target_include_directories(event_transformer INTERFACE include)

if(NOT BUILD_TESTING STREQUAL OFF)
  enable_testing()
  add_subdirectory(tests)
endif()

set_target_properties(event_transformer
                      PROPERTIES PUBLIC_HEADER "include/event_transformer.h")
install(TARGETS event_transformer)
