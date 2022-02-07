#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_MONOLITHIC_GAME)
    return()
endif()

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
