# CMake generated Testfile for 
# Source directory: C:/Users/Chris/Downloads/P3_starter
# Build directory: C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(p2_regression "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug/test_p2.exe")
set_tests_properties(p2_regression PROPERTIES  WORKING_DIRECTORY "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug" _BACKTRACE_TRIPLES "C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;82;add_test;C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;0;")
add_test(p3 "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug/test_p3.exe")
set_tests_properties(p3 PROPERTIES  WORKING_DIRECTORY "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug" _BACKTRACE_TRIPLES "C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;86;add_test;C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;0;")
add_test(tool_loop "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug/test_loop.exe")
set_tests_properties(tool_loop PROPERTIES  WORKING_DIRECTORY "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug" _BACKTRACE_TRIPLES "C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;90;add_test;C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;0;")
add_test(e2e "bash" "C:/Users/Chris/Downloads/P3_starter/tests/e2e/run_e2e.sh" "C:/Users/Chris/Downloads/P3_starter/out/build/x64-Debug/miniharness.exe")
set_tests_properties(e2e PROPERTIES  WORKING_DIRECTORY "C:/Users/Chris/Downloads/P3_starter" _BACKTRACE_TRIPLES "C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;99;add_test;C:/Users/Chris/Downloads/P3_starter/CMakeLists.txt;0;")
