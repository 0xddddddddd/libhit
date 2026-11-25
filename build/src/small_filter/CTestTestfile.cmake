# CMake generated Testfile for 
# Source directory: D:/code/libhit/src/small_filter
# Build directory: D:/code/libhit/build/src/small_filter
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("small_filter" "D:/code/libhit/build/src/small_filter/Debug/small_filter.exe")
  set_tests_properties("small_filter" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_filter" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_filter/Debug/small_filter.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_filter/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_filter/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("small_filter" "D:/code/libhit/build/src/small_filter/Release/small_filter.exe")
  set_tests_properties("small_filter" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_filter" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_filter/Release/small_filter.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_filter/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_filter/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("small_filter" "D:/code/libhit/build/src/small_filter/MinSizeRel/small_filter.exe")
  set_tests_properties("small_filter" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_filter" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_filter/MinSizeRel/small_filter.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_filter/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_filter/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("small_filter" "D:/code/libhit/build/src/small_filter/RelWithDebInfo/small_filter.exe")
  set_tests_properties("small_filter" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_filter" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_filter/RelWithDebInfo/small_filter.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_filter/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_filter/CMakeLists.txt;0;")
else()
  add_test("small_filter" NOT_AVAILABLE)
endif()
