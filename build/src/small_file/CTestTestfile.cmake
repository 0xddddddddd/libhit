# CMake generated Testfile for 
# Source directory: D:/code/libhit/src/small_file
# Build directory: D:/code/libhit/build/src/small_file
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("small_file" "D:/code/libhit/build/src/small_file/Debug/small_file.exe")
  set_tests_properties("small_file" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_file" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_file/Debug/small_file.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_file/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_file/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("small_file" "D:/code/libhit/build/src/small_file/Release/small_file.exe")
  set_tests_properties("small_file" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_file" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_file/Release/small_file.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_file/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_file/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("small_file" "D:/code/libhit/build/src/small_file/MinSizeRel/small_file.exe")
  set_tests_properties("small_file" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_file" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_file/MinSizeRel/small_file.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_file/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_file/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("small_file" "D:/code/libhit/build/src/small_file/RelWithDebInfo/small_file.exe")
  set_tests_properties("small_file" PROPERTIES  WORKING_DIRECTORY "D:/code/libhit/build/src/small_file" _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small_file/RelWithDebInfo/small_file.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small_file/CMakeLists.txt;9;add_test;D:/code/libhit/src/small_file/CMakeLists.txt;0;")
else()
  add_test("small_file" NOT_AVAILABLE)
endif()
