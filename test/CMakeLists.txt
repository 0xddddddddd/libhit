add_executable(libhit_benchmark benchmark.cc)
target_link_libraries(libhit_benchmark PRIVATE libhit benchmark::benchmark)
set_property(TARGET libhit_benchmark PROPERTY FOLDER "test")
