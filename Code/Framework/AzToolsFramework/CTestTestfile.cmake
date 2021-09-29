# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework
# Build directory: C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;92;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;92;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;92;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;95;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;95;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;95;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] NOT_AVAILABLE)
endif()
