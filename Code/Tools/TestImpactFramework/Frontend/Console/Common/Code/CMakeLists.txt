#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(common_dir "${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}")
ly_add_target(
    NAME TestImpact.Frontend.Console.Common.Headers HEADERONLY
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_common_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include/Headers
    BUILD_DEPENDENCIES
        INTERFACE
            AZ::AzCore
)

ly_add_target(
    NAME TestImpact.Frontend.Console.Common.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_common_files.cmake
    PLATFORM_INCLUDE_FILES
        "${common_dir}/testimpactframework_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake"
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include/Static
    COMPILE_DEFINITIONS
        PUBLIC
            ${LY_TEST_IMPACT_CONFIG_FILE_PATH_DEFINITION}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Runtime.Common.Headers
        PUBLIC
            AZ::TestImpact.Frontend.Console.Common.Headers
)
