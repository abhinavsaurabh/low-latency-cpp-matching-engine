cmake_minimum_required(VERSION 3.20)

project(
  hft_matching_engine
  VERSION 0.1.0
  DESCRIPTION "Low-latency C++ matching engine and order book simulator"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HFT_BUILD_TESTS "Build unit tests" ON)
option(HFT_BUILD_BENCHMARKS "Build benchmarks" ON)

add_library(
  hft_core
  src/order_book.cpp
  src/matching_engine.cpp
  src/risk.cpp
)

target_include_directories(hft_core PUBLIC include)

target_compile_options(
  hft_core
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)

add_executable(hft_demo apps/demo_main.cpp)
target_link_libraries(hft_demo PRIVATE hft_core)

if(HFT_BUILD_TESTS)
  enable_testing()
  add_executable(hft_tests tests/order_book_tests.cpp)
  target_link_libraries(hft_tests PRIVATE hft_core)
  add_test(NAME hft_tests COMMAND hft_tests)
endif()

if(HFT_BUILD_BENCHMARKS)
  add_executable(hft_bench bench/engine_bench.cpp)
  target_link_libraries(hft_bench PRIVATE hft_core)
endif()
