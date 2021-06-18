#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

if (NOT PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME AWSGameLift.Server.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_server_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ../AWSGameLiftCommon/Source
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            3rdParty::AWSGameLiftServerSDK
    )

ly_add_target(
    NAME AWSGameLift.Servers ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_server_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::AWSGameLift.Server.Static
            3rdParty::AWSGameLiftServerSDK
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSGameLift.Server.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            awsgamelift_server_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzTest
                Gem::AWSGameLift.Server.Static
                3rdParty::AWSGameLiftServerSDK
    )
    # Add AWSGameLift.Server.Tests to googletest
    ly_add_googletest(
        NAME Gem::AWSGameLift.Server.Tests
    )
endif()
