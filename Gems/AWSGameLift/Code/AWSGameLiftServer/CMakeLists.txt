#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    return()
endif()

o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)
o3de_pal_dir(pal_source_dir "${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME}" "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if (NOT PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED)
    return()
endif()


ly_add_target(
    NAME ${gem_name}.Server.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_server_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            ../AWSGameLiftCommon/Include
            Include
        PRIVATE
            ../AWSGameLiftCommon/Source
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::Multiplayer.Server.Static
            3rdParty::AWSGameLiftServerSDK
    )

ly_add_target(
    NAME ${gem_name}.Servers ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awsgamelift_server_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::${gem_name}.Server.Static
            3rdParty::AWSGameLiftServerSDK
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AWSGameLiftServerModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Server.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            awsgamelift_server_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                ../AWSGameLiftCommon/Include
                ../AWSGameLiftCommon/Source
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzTest
                Gem::Multiplayer.Server.Static
                Gem::${gem_name}.Server.Static
                3rdParty::AWSGameLiftServerSDK
    )
    # Add ${gem_name}.Server.Tests to googletest
    ly_add_googletest(
        NAME Gem::${gem_name}.Server.Tests
        LABELS REQUIRES_tiaf
    )
endif()
