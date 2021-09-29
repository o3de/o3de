# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests
# Build directory: C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/debug" "C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/unit/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RemoteConsole_UnitTests_main_no_gpu.xml")
  set_tests_properties([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;12;ly_add_pytest;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/profile" "C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/unit/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RemoteConsole_UnitTests_main_no_gpu.xml")
  set_tests_properties([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;12;ly_add_pytest;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/release" "C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/unit/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RemoteConsole_UnitTests_main_no_gpu.xml")
  set_tests_properties([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;12;ly_add_pytest;C:/Users/lesaelr/o3de/Tools/RemoteConsole/ly_remote_console/tests/CMakeLists.txt;0;")
else()
  add_test([=[RemoteConsole_UnitTests_main_no_gpu.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
