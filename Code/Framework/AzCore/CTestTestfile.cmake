# CMake generated Testfile for 
# Source directory: C:/Users/lesaelr/o3de/Code/Framework/AzCore
# Build directory: C:/Users/lesaelr/o3de/Code/Framework/AzCore
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;137;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;137;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:C:/Users/lesaelr/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;137;ly_add_googletest;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/debug/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/debug/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;140;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/profile/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/profile/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;140;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "C:/Users/lesaelr/o3de/bin/release/AzTestRunner.exe" "C:/Users/lesaelr/o3de/bin/release/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=C:/Users/lesaelr/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;147;add_test;C:/Users/lesaelr/o3de/cmake/LYTestWrappers.cmake;500;ly_add_test;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;140;ly_add_googlebenchmark;C:/Users/lesaelr/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] NOT_AVAILABLE)
endif()
