#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})
set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME TestImpact.Runtime.Python.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_runtime_python_files.cmake
    PLATFORM_INCLUDE_FILES
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/testimpactframework_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Runtime.Common.Static
        PUBLIC
            AZ::AzCore
            AZ::TestImpact.Runtime.Common.Headers
)

################################################################################
# Tests
################################################################################
 
# Disabled for future PRs
#ly_add_target( 
#    NAME TestImpact.Runtime.Python.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
#    NAMESPACE AZ
#    FILES_CMAKE
#        testimpactframework_runtime_python_tests_files.cmake
#    INCLUDE_DIRECTORIES
#        PRIVATE
#            Include
#            Source
#            Tests
#    BUILD_DEPENDENCIES
#        PRIVATE
#            AZ::AzTestShared
#            AZ::AzTest
#            AZ::TestImpact.Runtime.Python.Static
#            AZ::TestImpact.Runtime.Common.Static
#    RUNTIME_DEPENDENCIES
#        AZ::AzTestRunner
#    COMPILE_DEFINITIONS
#        PRIVATE
#            LY_TEST_IMPACT_AZ_TESTRUNNER_BIN="$<TARGET_FILE:AzTestRunner>"
#            LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Enum"
#            LY_TEST_IMPACT_TEST_TARGET_RESULTS_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Result"
#            LY_TEST_IMPACT_TEST_TARGET_COVERAGE_DIR="${GTEST_XML_OUTPUT_DIR}/TestImpact/Temp/Exclusive/Coverage"
#            LY_TEST_IMPACT_MODULES_DIR="${CMAKE_BINARY_DIR}"
#            LY_TEST_IMPACT_COVERAGE_SOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
#            LY_TEST_IMPACT_REPO_DIR="${CMAKE_SOURCE_DIR}"
#)
#
#ly_add_googletest(
#    NAME AZ::TestImpact.Runtime.Python.Tests
#)
