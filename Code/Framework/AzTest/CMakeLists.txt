#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    o3de_pal_dir(pal_aztest_dir ${CMAKE_CURRENT_LIST_DIR}/AzTest/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 2fe3bd994b3189899d93f1d5a881e725e046fdc2 # release-1.8.1
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # Prevent overriding the parent project's compiler/linker settings
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE) # This setting builds gtest and gmock
    set(BUILD_GTEST OFF CACHE BOOL "" FORCE) # This setting would only build gtest
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE) # Install target is not needed
    FetchContent_MakeAvailable(googletest)

    ly_create_alias(NAME googletest::GTest NAMESPACE 3rdParty TARGETS gtest)
    ly_create_alias(NAME googletest::GMock NAMESPACE 3rdParty TARGETS gmock)

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
                3rdParty::googletest::GMock
                3rdParty::googletest::GTest
                3rdParty::GoogleBenchmark
                AZ::AzCore
        PLATFORM_INCLUDE_FILES
            ${pal_aztest_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    )
endif()
