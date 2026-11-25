add_executable(small main.cc)
target_include_directories(small PRIVATE ${PROJECT_SOURCE_DIR}/include)
set_property(TARGET small PROPERTY FOLDER "example")
add_test(NAME small COMMAND small)
