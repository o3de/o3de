#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(awsgameliftclient_compile_definition $<IF:$<CONFIG:release>,AWSGAMELIFT_RELEASE,AWSGAMELIFT_DEV>)

ly_add_target(
    NAME AWSGameLift.Client.Static STATIC
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
            AZ::AzFramework
            Gem::AWSCore
            3rdParty::AWSNativeSDK::GameLiftClient
)

ly_add_target(
    NAME AWSGameLift.Clients ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_client_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            3rdParty::AWSNativeSDK::GameLiftClient
        PUBLIC
            Gem::AWSGameLift.Client.Static
    RUNTIME_DEPENDENCIES
        Gem::AWSCore.Clients
)

# Load the "Gem::AWSGameLift" module in all types of applications.
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(
        NAME AWSGameLift.Tools
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Tools
            Gem::AWSGameLift.Clients
    )

    ly_create_alias(
        NAME AWSGameLift.Builders
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Builders
            Gem::AWSGameLift.Clients
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSGameLift.Client.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                AZ::AzFramework
                AZ::AzTest
                Gem::AWSCore
                Gem::AWSGameLift.Client.Static
                3rdParty::AWSNativeSDK::GameLiftClient
                AZ::AWSNativeSDKInit
    )
    # Add AWSGameLift.Client.Tests to googletest
    ly_add_googletest(
        NAME Gem::AWSGameLift.Client.Tests
    )
endif()
