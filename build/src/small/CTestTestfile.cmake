# CMake generated Testfile for 
# Source directory: D:/code/libhit/src/small
# Build directory: D:/code/libhit/build/src/small
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("small" "D:/code/libhit/build/src/small/Debug/small.exe")
  set_tests_properties("small" PROPERTIES  _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small/Debug/small.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small/CMakeLists.txt;3;add_test;D:/code/libhit/src/small/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("small" "D:/code/libhit/build/src/small/Release/small.exe")
  set_tests_properties("small" PROPERTIES  _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small/Release/small.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small/CMakeLists.txt;3;add_test;D:/code/libhit/src/small/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("small" "D:/code/libhit/build/src/small/MinSizeRel/small.exe")
  set_tests_properties("small" PROPERTIES  _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small/MinSizeRel/small.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small/CMakeLists.txt;3;add_test;D:/code/libhit/src/small/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("small" "D:/code/libhit/build/src/small/RelWithDebInfo/small.exe")
  set_tests_properties("small" PROPERTIES  _CMAKE_TEST_BUILD_DEPENDS "D:/code/libhit/build/src/small/RelWithDebInfo/small.exe" _BACKTRACE_TRIPLES "D:/code/libhit/src/small/CMakeLists.txt;3;add_test;D:/code/libhit/src/small/CMakeLists.txt;0;")
else()
  add_test("small" NOT_AVAILABLE)
endif()
