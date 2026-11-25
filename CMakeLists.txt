cmake_minimum_required(VERSION 3.20)
project(libhit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Benchmark v1.10.0 is kept in third-party/benchmark. Fetch it on configure
# only when the local checkout is absent, while allowing an offline override.
option(LIBHIT_FETCH_BENCHMARK "Fetch Google Benchmark v1.10.0 if missing" ON)
set(LIBHIT_BENCHMARK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/benchmark"
    CACHE PATH "Local Google Benchmark source directory")

if(NOT EXISTS "${LIBHIT_BENCHMARK_SOURCE_DIR}/CMakeLists.txt")
    if(NOT LIBHIT_FETCH_BENCHMARK)
        message(FATAL_ERROR
            "Google Benchmark is missing at '${LIBHIT_BENCHMARK_SOURCE_DIR}'. "
            "Enable LIBHIT_FETCH_BENCHMARK or provide the source directory.")
    endif()
    include(FetchContent)
    FetchContent_Declare(google_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.10.0
        GIT_SHALLOW TRUE
        SOURCE_DIR "${LIBHIT_BENCHMARK_SOURCE_DIR}"
    )
    FetchContent_GetProperties(google_benchmark)
    if(NOT google_benchmark_POPULATED)
        FetchContent_Populate(google_benchmark)
    endif()
endif()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${LIBHIT_BENCHMARK_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/third-party/benchmark" EXCLUDE_FROM_ALL)

# Shared header-only library target used by the independent example projects.
add_library(libhit INTERFACE)
target_include_directories(libhit INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(src/small)
add_subdirectory(src/small_file)
add_subdirectory(src/small_filter)
add_subdirectory(test)
