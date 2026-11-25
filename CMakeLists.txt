cmake_minimum_required(VERSION 3.20)

# Select 32-bit Visual Studio builds by default when the caller did not choose
# an architecture. This must happen before project() enables the C++ toolchain.
if(WIN32 AND NOT DEFINED CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM Win32 CACHE STRING "Target platform" FORCE)
endif()

project(libhit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Organize generated Visual Studio targets into solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

# Benchmark v1.10.0 is kept in third-party/benchmark. Fetch it on configure
# only when the local checkout is absent, while allowing an offline override.
option(LIBHIT_FETCH_BENCHMARK "Fetch Google Benchmark v1.10.0 if missing" ON)
set(LIBHIT_BENCHMARK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/benchmark"
    CACHE PATH "Local Google Benchmark source directory")

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

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
    FetchContent_MakeAvailable(google_benchmark)
else()
    add_subdirectory("${LIBHIT_BENCHMARK_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/third-party/benchmark" EXCLUDE_FROM_ALL)
endif()

# Keep all Google Benchmark targets, including targets in nested CMake
# directories, under a dedicated Visual Studio solution folder.
function(libhit_set_directory_targets_folder directory folder_name)
    get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target IN LISTS _targets)
        set_property(TARGET "${_target}" PROPERTY FOLDER "${folder_name}")
    endforeach()

    get_property(_subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_subdirectory IN LISTS _subdirectories)
        libhit_set_directory_targets_folder("${_subdirectory}" "${folder_name}")
    endforeach()
endfunction()
libhit_set_directory_targets_folder("${LIBHIT_BENCHMARK_SOURCE_DIR}" "third-party")

# Shared header-only library target used by the independent example projects.
add_library(libhit INTERFACE)
target_include_directories(libhit INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(src/small)
add_subdirectory(src/small_file)
add_subdirectory(src/small_filter)
add_subdirectory(test)
