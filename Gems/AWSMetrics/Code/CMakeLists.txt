#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsmetrics_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::AWSCore
            3rdParty::AWSNativeSDK::Core
)

ly_add_target( 
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsmetrics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AWSMetricsModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Load the "Gem::${gem_name}" module in all types of applications.
ly_create_alias(
    NAME ${gem_name}.Servers
    NAMESPACE Gem
    TARGETS
        Gem::AWSCore.Servers
        Gem::${gem_name}
)

ly_create_alias(
    NAME ${gem_name}.Clients
    NAMESPACE Gem
    TARGETS
        Gem::AWSCore.Clients
        Gem::${gem_name}
)

ly_create_alias(
    NAME ${gem_name}.Unified
    NAMESPACE Gem
    TARGETS
        Gem::AWSCore.Unified
        Gem::${gem_name}
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(
        NAME ${gem_name}.Tools
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Tools
            Gem::${gem_name}.Editor
    )

    ly_create_alias(
        NAME ${gem_name}.Builders
        NAMESPACE Gem
        TARGETS
            Gem::AWSCore.Builders
            Gem::${gem_name}
    )
endif()

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target( 
    NAME ${gem_name}.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsmetrics_editor_files.cmake
        awsmetrics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            AWS_METRICS_EDITOR
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            Gem::AWSCore
            3rdParty::AWSNativeSDK::Core
    RUNTIME_DEPENDENCIES
            Gem::${gem_name}
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            awsmetrics_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzCore
                AZ::AzFramework
                Gem::${gem_name}.Static
        RUNTIME_DEPENDENCIES
            Gem::AWSCore
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )
endif()
