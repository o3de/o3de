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

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME AzManipulatorTestFramework.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        azmanipulatortestframework_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzTest
            AZ::AzToolsFramework
            AZ::AzToolsFrameworkTestCommon
            AZ::AzTestShared
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME AzManipulatorTestFramework.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
    NAMESPACE AZ
    FILES_CMAKE
        azmanipulatortestframework_tests_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Tests
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzManipulatorTestFramework.Static
)

ly_add_googletest(
    NAME AZ::AzManipulatorTestFramework.Tests
)
