add_executable(small_filter main.cc)
target_link_libraries(small_filter PRIVATE libhit)
set_property(TARGET small_filter PROPERTY FOLDER "example")

set(test_data "${PROJECT_SOURCE_DIR}/vsprojects/small_filter/test_data.bin")
if(EXISTS "${test_data}")
    configure_file("${test_data}" "${CMAKE_CURRENT_BINARY_DIR}/test_data.bin" COPYONLY)
endif()

add_test(NAME small_filter COMMAND small_filter)
set_tests_properties(small_filter PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS small_filter RUNTIME DESTINATION .)