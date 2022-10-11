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
