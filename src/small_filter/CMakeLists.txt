add_executable(small_filter main.cc)
target_include_directories(small_filter PRIVATE ${PROJECT_SOURCE_DIR}/include)

set(test_data "${PROJECT_SOURCE_DIR}/vsprojects/small_filter/test_data.bin")
if(EXISTS "${test_data}")
    configure_file("${test_data}" "${CMAKE_CURRENT_BINARY_DIR}/test_data.bin" COPYONLY)
endif()

add_test(NAME small_filter COMMAND small_filter)
set_tests_properties(small_filter PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
