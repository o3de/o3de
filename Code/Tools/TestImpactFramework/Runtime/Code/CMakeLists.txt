#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME TestImpact.Runtime.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_runtime_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
     PLATFORM_INCLUDE_FILES
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/testimpactframework_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
)

################################################################################
# Tests
################################################################################

# Disabled: SPEC-7246
#add_subdirectory(Tests/TestProcess)
#add_subdirectory(Tests/TestTargetA)
#add_subdirectory(Tests/TestTargetB)
#add_subdirectory(Tests/TestTargetC)
#add_subdirectory(Tests/TestTargetD)
#
#ly_add_target(
#    NAME TestImpact.Runtime.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
#    NAMESPACE AZ
#    FILES_CMAKE
#        testimpactframework_runtime_tests_files.cmake
#    INCLUDE_DIRECTORIES
#        PRIVATE
#            Include
#            Source
#            Tests
#    BUILD_DEPENDENCIES
#        PRIVATE
#            AZ::AzTestShared
#            AZ::AzTest
#            AZ::TestImpact.Runtime.Static
#    RUNTIME_DEPENDENCIES
#        AZ::AzTestRunner
#        AZ::TestImpact.TestProcess.Console
#        AZ::TestImpact.TestTargetA.Tests
#        AZ::TestImpact.TestTargetB.Tests
#        AZ::TestImpact.TestTargetC.Tests
#        AZ::TestImpact.TestTargetD.Tests
#    COMPILE_DEFINITIONS
#        PRIVATE
#            LY_TEST_IMPACT_AZ_TESTRUNNER_BIN="$<TARGET_FILE:AzTestRunner>"
#            LY_TEST_IMPACT_TEST_PROCESS_BIN="$<TARGET_FILE:TestImpact.TestProcess.Console>"
#            LY_TEST_IMPACT_TEST_TARGET_A_BIN="$<TARGET_FILE:TestImpact.TestTargetA.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_B_BIN="$<TARGET_FILE:TestImpact.TestTargetB.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_C_BIN="$<TARGET_FILE:TestImpact.TestTargetC.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_D_BIN="$<TARGET_FILE:TestImpact.TestTargetD.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_A_BASE_NAME="$<TARGET_FILE_BASE_NAME:TestImpact.TestTargetA.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_B_BASE_NAME="$<TARGET_FILE_BASE_NAME:TestImpact.TestTargetB.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_C_BASE_NAME="$<TARGET_FILE_BASE_NAME:TestImpact.TestTargetC.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_D_BASE_NAME="$<TARGET_FILE_BASE_NAME:TestImpact.TestTargetD.Tests>"
#            LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Enum"
#            LY_TEST_IMPACT_TEST_TARGET_RESULTS_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Result"
#            LY_TEST_IMPACT_TEST_TARGET_COVERAGE_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Coverage"
#            LY_TEST_IMPACT_INSTRUMENTATION_BIN="${LY_TEST_IMPACT_INSTRUMENTATION_BIN}"
#            LY_TEST_IMPACT_MODULES_DIR="${CMAKE_BINARY_DIR}"
#            LY_TEST_IMPACT_COVERAGE_SOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
#)
#
#ly_add_googletest(
#    NAME AZ::TestImpact.Runtime.Tests
#)
