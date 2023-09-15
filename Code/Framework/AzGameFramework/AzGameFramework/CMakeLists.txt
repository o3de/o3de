#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AzGameFramework STATIC
    NAMESPACE AZ
    FILES_CMAKE
        azgameframework_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            ..
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AzFramework.NativeUI
)

# The 'Headless' version AzGameFramework is the same as the original, with the exception of 2 things:
# 1. An 'O3DE_HEADLESS_SERVER' define that controls headless specific sections of the shared source files
# 2. The dependency on 'AZ::AzFramework.NativeUI' is removed to ensure we don't pull in any UI dependencies
ly_add_target(
    NAME AzGameFramework.Headless STATIC
    NAMESPACE AZ
    FILES_CMAKE
        azgameframework_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            ..
    COMPILE_DEFINITIONS
        PUBLIC
            O3DE_HEADLESS_SERVER=1
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)
