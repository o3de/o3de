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
    RUNTIME_DEPENDENCIES
        Gem::AWSCore
)

# Load the "Gem::AWSMetrics" module in all types of applications.
ly_create_alias(NAME AWSMetrics.Servers NAMESPACE Gem TARGETS Gem::AWSMetrics)
ly_create_alias(NAME AWSMetrics.Clients NAMESPACE Gem TARGETS Gem::AWSMetrics)
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AWSMetrics.Tools NAMESPACE Gem TARGETS Gem::AWSMetrics)
    ly_create_alias(NAME AWSMetrics.Builders NAMESPACE Gem TARGETS Gem::AWSMetrics)
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
