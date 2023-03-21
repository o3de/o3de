#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

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

if(PAL_TRAIT_BUILD_HOST_TOOLS)

ly_add_target( 
    NAME AWSClientAuth.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsclientauth_editor_files.cmake
        awsclientauth_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            AWSCLIENTAUTH_EDITOR
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            Gem::AWSCore
            Gem::HttpRequestor
            3rdParty::AWSNativeSDK::AWSClientAuth
            3rdParty::AWSNativeSDK::Core
    RUNTIME_DEPENDENCIES
        Gem::AWSClientAuth
)

endif()


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

ly_create_alias(
    NAME AWSClientAuth.Unified
    NAMESPACE Gem
    TARGETS
        Gem::AWSClientAuth
        Gem::AWSCore.Unified
        Gem::HttpRequestor.Unified
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(
        NAME AWSClientAuth.Tools
        NAMESPACE Gem
        TARGETS
            Gem::AWSClientAuth.Editor
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
                AZ::AWSNativeSDKTestLibs
                Gem::AWSClientAuth.Static
                Gem::AWSCore
                Gem::HttpRequestor
        RUNTIME_DEPENDENCIES
            Gem::AWSCore
            Gem::HttpRequestor
   )
   ly_add_googletest(
        NAME Gem::AWSClientAuth.Tests
        LABELS REQUIRES_tiaf
    )
endif()
