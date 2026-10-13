cmake_minimum_required(VERSION 3.16)
project(CoreBench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(COREBENCH_GBENCH "Build the Google Benchmark suite (downloads Google Benchmark)" ON)

find_package(Threads REQUIRED)

add_library(corebench_lib src/workloads.cpp)
target_include_directories(corebench_lib PUBLIC include)
target_compile_options(corebench_lib PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(corebench_lib PUBLIC Threads::Threads)

add_executable(corebench src/main.cpp)
target_compile_options(corebench PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(corebench PRIVATE corebench_lib)

enable_testing()
add_executable(corebench_tests tests/test_workloads.cpp)
target_link_libraries(corebench_tests PRIVATE corebench_lib)
add_test(NAME corebench_tests COMMAND corebench_tests)

if(COREBENCH_GBENCH)
  find_package(benchmark QUIET)
  if(NOT benchmark_FOUND)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.8.3)
    FetchContent_MakeAvailable(benchmark)
  endif()
  add_executable(corebench_gbench bench/gbench.cpp)
  target_link_libraries(corebench_gbench PRIVATE corebench_lib benchmark::benchmark)
endif()
