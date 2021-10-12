# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Gems/Blast/Code
# Build directory: C:/Users/lesaelr/o3de/External/Blast-dc3b9232/Code
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/Blast.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Tests.xml" "--gtest_filter=*SUITE_sandbox*")
  set_tests_properties([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] PROPERTIES  LABELS "SUITE_sandbox;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;143;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/Blast.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Tests.xml" "--gtest_filter=*SUITE_sandbox*")
  set_tests_properties([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] PROPERTIES  LABELS "SUITE_sandbox;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;143;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/Blast.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Tests.xml" "--gtest_filter=*SUITE_sandbox*")
  set_tests_properties([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] PROPERTIES  LABELS "SUITE_sandbox;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;143;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
else()
  add_test([=[Gem::Blast.Tests.sandbox::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/Blast.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;168;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/Blast.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;168;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/Blast.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/Gem_Blast.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;168;ly_add_googletest;C:/Users/lesaelr/o3de/Gems/Blast/Code/CMakeLists.txt;0;")
else()
  add_test([=[Gem::Blast.Editor.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
