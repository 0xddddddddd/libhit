add_executable(small_file main.cc)
target_link_libraries(small_file PRIVATE libhit)
set_property(TARGET small_file PROPERTY FOLDER "example")

set(test_data "${PROJECT_SOURCE_DIR}/vsprojects/small_file/test_data.bin")
if(EXISTS "${test_data}")
    configure_file("${test_data}" "${CMAKE_CURRENT_BINARY_DIR}/test_data.bin" COPYONLY)
endif()

add_test(NAME small_file COMMAND small_file)
set_tests_properties(small_file PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    
install(TARGETS small_file RUNTIME DESTINATION .)
