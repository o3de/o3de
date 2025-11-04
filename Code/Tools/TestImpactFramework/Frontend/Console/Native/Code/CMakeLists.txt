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

set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME TestImpact.Frontend.Console.Native.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_native_static_files.cmake
    PLATFORM_INCLUDE_FILES
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/testimpactframework_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Frontend.Console.Common.Static
            AZ::TestImpact.Runtime.Native.Static
        PUBLIC
            AZ::TestImpact.Frontend.Console.Common.Headers
)

ly_add_target(
    NAME TestImpact.Frontend.Console.Native EXECUTABLE
    OUTPUT_NAME tiaf_native
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_native_files.cmake
    PLATFORM_INCLUDE_FILES
        ${common_dir}/${PAL_TRAIT_COMPILER_ID}/testimpactframework_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Frontend.Console.Native.Static
)
