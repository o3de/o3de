#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME AWSClientAuth.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsclientauth_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::AWSCore
            Gem::HttpRequestor
            3rdParty::AWSNativeSDK::AWSClientAuth
            3rdParty::AWSNativeSDK::Core
)

ly_add_target( 
    NAME AWSClientAuth ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsclientauth_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::AWSCore
            3rdParty::AWSNativeSDK::AWSClientAuth
            3rdParty::AWSNativeSDK::Core
        PUBLIC
            Gem::AWSClientAuth.Static
)

# Load the "Gem::AWSClientAuth" module in all types of applications.
ly_create_alias(
    NAME AWSClientAuth.Servers
    NAMESPACE Gem
    TARGETS
        Gem::AWSClientAuth
        Gem::AWSCore.Servers
        Gem::HttpRequestor.Servers
)

ly_create_alias(
    NAME AWSClientAuth.Clients
    NAMESPACE Gem
    TARGETS
        Gem::AWSClientAuth
        Gem::AWSCore.Clients
        Gem::HttpRequestor.Clients
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(
        NAME AWSClientAuth.Tools
        NAMESPACE Gem
        TARGETS
            Gem::AWSClientAuth
            Gem::AWSCore.Tools
            Gem::HttpRequestor.Tools
    )

    ly_create_alias(
        NAME AWSClientAuth.Builders
        NAMESPACE Gem
        TARGETS
            Gem::AWSClientAuth
            Gem::AWSCore.Builders
            Gem::HttpRequestor.Builders
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSClientAuth.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            awsclientauth_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                3rdParty::AWSNativeSDK::AWSClientAuth
                AZ::AzCore
                AZ::AzFramework
                AZ::AWSNativeSDKInit
                Gem::AWSClientAuth.Static
                Gem::AWSCore
                Gem::HttpRequestor
        RUNTIME_DEPENDENCIES
            Gem::AWSCore
            AZ::AWSNativeSDKInit
            Gem::HttpRequestor
   )
   ly_add_googletest(
        NAME Gem::AWSClientAuth.Tests
    )
endif()
