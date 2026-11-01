cmake_minimum_required(VERSION 3.14)
project(split LANGUAGES CXX)

add_library(split INTERFACE)
add_library(split::split ALIAS split)
target_include_directories(split INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(split INTERFACE cxx_std_17)

# Only build the smoke test when this repo is the top-level project, so
# consumers (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(split_test test/test.cc)
  target_link_libraries(split_test PRIVATE split)
  add_test(NAME split COMMAND split_test)

  # A runnable tour of the library (not a test): build & run ./build/split_demo
  add_executable(split_demo examples/demo.cc)
  target_link_libraries(split_demo PRIVATE split)

  # Comparison benchmark (not run by ctest): split against memchr and find loops and, when abseil
  # is installed, absl::StrSplit. The comparator is optional and never a dependency of the
  # library target. With the option on, the program also runs in `--check` mode under ctest
  # (results, no timing). Configure with -DCMAKE_BUILD_TYPE=Release.
  option(SPLIT_BUILD_BENCH "Build bench/compare.cc" OFF)
  if(SPLIT_BUILD_BENCH)
    find_package(absl QUIET)
    add_executable(split_compare bench/compare.cc)
    target_link_libraries(split_compare PRIVATE split)
    # Split's iterator derives from std::iterator, which C++17 deprecates.
    target_compile_options(split_compare PRIVATE -Wno-deprecated-declarations)
    if(absl_FOUND)
      target_compile_definitions(split_compare PRIVATE HAVE_ABSL=1)
      target_link_libraries(split_compare PRIVATE absl::strings)
    endif()
    add_test(NAME split_compare COMMAND split_compare --check)
  endif()
endif()
