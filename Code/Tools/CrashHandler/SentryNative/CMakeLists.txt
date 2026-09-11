#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Shared, Qt-free wiring used by both this tools layer and the CrashReporting gem's runtime path.
add_subdirectory(Core)

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ToolsCrashHandler STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sentry_native_crash_handler_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::SentryCrashCore
            AZ::AzCore
)

add_subdirectory(Reporter)
