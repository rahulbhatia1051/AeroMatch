cmake_minimum_required(VERSION 3.14)
project(AeroMatch LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(aeromatch INTERFACE)
target_include_directories(aeromatch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(aeromatch INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(aeromatch_engine src/main.cpp)
target_link_libraries(aeromatch_engine PRIVATE aeromatch)

include(CTest)
if(BUILD_TESTING)
  add_executable(aeromatch_tests tests/order_book_tests.cpp)
  target_link_libraries(aeromatch_tests PRIVATE aeromatch)
  add_test(NAME aeromatch_tests COMMAND aeromatch_tests)
endif()

# Keep the default demo build self-contained. Enable this explicitly when a
# Google Benchmark run is required; the dependency is fetched at configure time.
option(AEROMATCH_BUILD_BENCHMARKS "Build Google Benchmark targets" OFF)
if(AEROMATCH_BUILD_BENCHMARKS)
  include(FetchContent)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
    GIT_SHALLOW TRUE
  )

  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(benchmark)

  add_executable(aeromatch_bench src/benchmark.cpp)
  target_link_libraries(aeromatch_bench PRIVATE aeromatch benchmark::benchmark)
endif()
