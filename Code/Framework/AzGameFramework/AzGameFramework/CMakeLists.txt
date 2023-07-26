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
            LY_HEADLESS=1
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)


