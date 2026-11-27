add_executable(small_single_byte main.cc)
target_link_libraries(small_single_byte PRIVATE libhit)
set_property(TARGET small_single_byte PROPERTY FOLDER "example")
add_test(NAME small_single_byte COMMAND small_single_byte)

install(TARGETS small_single_byte RUNTIME DESTINATION .)
