cmake_minimum_required(VERSION 3.14)
project(split LANGUAGES CXX)

add_library(split INTERFACE)
target_include_directories(split INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(split INTERFACE cxx_std_17)

enable_testing()
add_executable(split_test test/test.cc)
target_link_libraries(split_test PRIVATE split)
add_test(NAME split COMMAND split_test)
