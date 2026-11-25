add_executable(small main.cc)
target_link_libraries(small PRIVATE libhit)
set_property(TARGET small PROPERTY FOLDER "example")
add_test(NAME small COMMAND small)

install(TARGETS small RUNTIME DESTINATION .)