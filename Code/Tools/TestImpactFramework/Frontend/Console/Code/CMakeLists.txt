#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

ly_add_target(
    NAME TestImpact.Frontend.Console.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_static_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            ${LY_TEST_IMPACT_CONFIG_FILE_PATH_DEFINITION}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::TestImpact.Runtime.Static
)

ly_add_target(
    NAME TestImpact.Frontend.Console EXECUTABLE
    OUTPUT_NAME tiaf
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Frontend.Console.Static
)

################################################################################
# Tests
################################################################################

# Disbled:SPEC-7246
#ly_add_target(
#    NAME TestImpact.Frontend.Console.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
#    NAMESPACE AZ
#    FILES_CMAKE
#        testimpactframework_frontend_console_static_tests_files.cmake
#    INCLUDE_DIRECTORIES
#        PRIVATE
#            Include
#            Source
#            Tests
#    BUILD_DEPENDENCIES
#        PRIVATE
#            AZ::AzTestShared
#            AZ::AzTest
#            AZ::TestImpact.Frontend.Console.Static
#)
#
#ly_add_googletest(
#    NAME AZ::TestImpact.Frontend.Console.Static.Tests
#)
