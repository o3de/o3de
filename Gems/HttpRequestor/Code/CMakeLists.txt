#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(source_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME HttpRequestor.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        httprequestor_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AWSNativeSDKInit
        PUBLIC
            3rdParty::AWSNativeSDK::Core
)

ly_add_target(
    NAME HttpRequestor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        httprequestor_shared_files.cmake
    PLATFORM_INCLUDE_FILES
        ${source_pal_dir}/httprequestor_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::HttpRequestor.Static
)

# Load the "Gem::HttpRequestor" module in all types of applicatons.
ly_create_alias(NAME HttpRequestor.Clients  NAMESPACE Gem TARGETS Gem::HttpRequestor)
ly_create_alias(NAME HttpRequestor.Servers  NAMESPACE Gem TARGETS Gem::HttpRequestor)
ly_create_alias(NAME HttpRequestor.Builders NAMESPACE Gem TARGETS Gem::HttpRequestor)
ly_create_alias(NAME HttpRequestor.Tools    NAMESPACE Gem TARGETS Gem::HttpRequestor)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME HttpRequestor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            httprequestor_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::HttpRequestor.Static
    )
    ly_add_googletest(
        NAME Gem::HttpRequestor.Tests
        TEST_SUITE sandbox
    )
endif()
