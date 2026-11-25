add_executable(small main.cc)
target_include_directories(small PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME small COMMAND small)
