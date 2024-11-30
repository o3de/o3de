#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.Client.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_client_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            ../AWSGameLiftCommon/Include
            Include
        PRIVATE
            ../AWSGameLiftCommon/Source
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            ${awsgameliftclient_compile_definition}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::AWSCore
            Gem::Multiplayer.Unified.Static
            3rdParty::AWSNativeSDK::GameLiftClient
)

ly_add_target(
    NAME ${gem_name}.Clients ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_client_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Multiplayer.Common.Static
            3rdParty::AWSNativeSDK::GameLiftClient
        PUBLIC
            Gem::${gem_name}.Client.Static
    RUNTIME_DEPENDENCIES
        Gem::AWSCore.Clients
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AWSGameLiftClientModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Load the "Gem::${gem_name}" module in all types of applications.
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            awsgamelift_client_editor_files.cmake
            awsgamelift_client_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                ../AWSGameLiftCommon/Include
                ../AWSGameLiftCommon/Source
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                AWS_GAMELIFT_CLIENT_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::AWSCore
                Gem::Multiplayer.Unified.Static
                3rdParty::AWSNativeSDK::GameLiftClient
        RUNTIME_DEPENDENCIES
                Gem::${gem_name}.Clients
    )

    ly_create_alias(
        NAME ${gem_name}.Tools
        NAMESPACE Gem
        TARGETS
            Gem::${gem_name}.Editor
            Gem::AWSCore.Tools
    )

    ly_create_alias(
        NAME ${gem_name}.Builders
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Builders
            Gem::${gem_name}.Clients
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Client.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            awsgamelift_client_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                ../AWSGameLiftCommon/Include
                ../AWSGameLiftCommon/Source
                Include
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzTest
                Gem::AWSCore
                Gem::${gem_name}.Client.Static
                Gem::Multiplayer.Unified.Static
                3rdParty::AWSNativeSDK::GameLiftClient
                AZ::AWSNativeSDKTestLibs
    )
    # Add ${gem_name}.Client.Tests to googletest
    ly_add_googletest(
        NAME Gem::${gem_name}.Client.Tests
        LABELS REQUIRES_tiaf
    )
endif()
