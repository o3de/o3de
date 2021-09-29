# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools
# Build directory: C:/Users/lesaelr/o3de/External/Atom-418206cc/RPI/Tools
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/debug" "C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/profile" "C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "C:/Users/lesaelr/o3de/ctest_pytest.ini" "--build-directory" "C:/Users/lesaelr/o3de/bin/release" "C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=C:/Users/lesaelr/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;C:/Users/lesaelr/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
else()
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
