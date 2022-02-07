#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AWSMetrics.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsmetrics_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include/Public
        PRIVATE
            Include/Private
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::AWSCore
            3rdParty::AWSNativeSDK::Core
)

ly_add_target( 
    NAME AWSMetrics ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsmetrics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include/Public
        PRIVATE
            Include/Private
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::AWSMetrics.Static
)

# Load the "Gem::AWSMetrics" module in all types of applications.
ly_create_alias(
    NAME AWSMetrics.Servers
    NAMESPACE Gem
    TARGETS
        Gem::AWSCore.Servers
        Gem::AWSMetrics
)

ly_create_alias(
    NAME AWSMetrics.Clients
    NAMESPACE Gem
    TARGETS
        Gem::AWSCore.Clients
        Gem::AWSMetrics
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(
        NAME AWSMetrics.Tools
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Tools
            Gem::AWSMetrics
    )

    ly_create_alias(
        NAME AWSMetrics.Builders
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Builders
            Gem::AWSMetrics
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSMetrics.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            awsmetrics_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include/Private
                Include/Public
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzCore
                AZ::AzFramework
                Gem::AWSMetrics.Static
        RUNTIME_DEPENDENCIES
            Gem::AWSCore
    )
    ly_add_googletest(
        NAME Gem::AWSMetrics.Tests
    )
endif()
