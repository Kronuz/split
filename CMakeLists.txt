cmake_minimum_required(VERSION 3.14)
project(split LANGUAGES CXX)

add_library(split INTERFACE)
target_include_directories(split INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(split INTERFACE cxx_std_17)

# Only build the smoke test when this repo is the top-level project, so
# consumers (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(split_test test/test.cc)
  target_link_libraries(split_test PRIVATE split)
  add_test(NAME split COMMAND split_test)
endif()
