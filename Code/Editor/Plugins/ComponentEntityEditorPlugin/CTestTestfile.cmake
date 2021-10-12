# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin
# Build directory: C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/ComponentEntityEditorPlugin.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Legacy_ComponentEntityEditorPlugin.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;75;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/ComponentEntityEditorPlugin.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Legacy_ComponentEntityEditorPlugin.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;75;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/ComponentEntityEditorPlugin.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Legacy_ComponentEntityEditorPlugin.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;75;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Editor/Plugins/ComponentEntityEditorPlugin/CMakeLists.txt;0;")
else()
  add_test([=[Legacy::ComponentEntityEditorPlugin.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
