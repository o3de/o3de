#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT LY_MONOLITHIC_GAME)
    o3de_pal_dir(aztest_pal_dir ${CMAKE_CURRENT_LIST_DIR}/AzTest/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

    ly_add_target(
        NAME AzTest STATIC
        NAMESPACE AZ
        FILES_CMAKE
            AzTest/aztest_files.cmake
            ${aztest_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ${pal_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::googletest::GMock
                3rdParty::googletest::GTest
                3rdParty::GoogleBenchmark
                AZ::AzCore
        PLATFORM_INCLUDE_FILES
            ${aztest_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    )
endif()
