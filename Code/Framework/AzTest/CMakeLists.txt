#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Note we do not use if(PAL_TRAIT_BUILD_TESTS_SUPPORTED) on this file.
# This is because that flag is used to determine if the tests themselves should be built or not
# But this is how you would build your own tests, and must be included in the installer, even if
# we don't include the tests in the installer.

# when compiling from source, use the 3rdParty/Find***.cmake files
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

o3de_pal_dir(pal_aztest_dir ${CMAKE_CURRENT_LIST_DIR}/AzTest/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

if (PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
    ly_add_target(
        NAME AzTest STATIC
        NAMESPACE AZ
        FILES_CMAKE
            AzTest/aztest_files.cmake
            ${pal_aztest_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                ${pal_aztest_dir}
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::googletest::GMock   # This dependency is what causes the find files to run.
                3rdParty::googletest::GTest
                3rdParty::GoogleBenchmark
                AZ::AzCore
        PLATFORM_INCLUDE_FILES
            ${pal_aztest_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    )
endif()
