#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/source/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

ly_add_target(
    NAME AWSNativeSDKInit STATIC
    NAMESPACE AZ
    FILES_CMAKE
        aws_native_sdk_init_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
        PRIVATE
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::AWSNativeSDK::Core
            AZ::AzCore
)

ly_add_target(
    NAME AWSNativeSDKTestLibs STATIC
    NAMESPACE AZ
    FILES_CMAKE
        aws_native_sdk_test_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
            tests/libs
        PRIVATE
            source
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::AWSNativeSDK::Core
            AZ::AzCore
            AZ::AzTest
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSNativeSDKInit.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            aws_native_sdk_init_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                include
                tests
                source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzTest
                AZ::AWSNativeSDKInit
                3rdParty::AWSNativeSDK::Core
    )
    ly_add_googletest(
        NAME AZ::AWSNativeSDKInit.Tests
    )
endif()
