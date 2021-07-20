#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CrashSupport STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_handler_support_files.cmake
        Platform/${PAL_PLATFORM_NAME}/crash_handler_support_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
)
