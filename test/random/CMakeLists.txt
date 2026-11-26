add_executable(libhit_random_test random.cc)
target_link_libraries(libhit_random_test PRIVATE libhit)
set_property(TARGET libhit_random_test PROPERTY FOLDER "test")
add_test(NAME libhit_random_test COMMAND libhit_random_test)
