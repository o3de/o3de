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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_tests_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/Common)

ly_add_target(
    NAME Multiplayer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        multiplayer_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pal_source_dir}
            Source
            .
        PUBLIC
            Include
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Legacy::CryNetwork.Static
            AZ::GridMate
            Gem::GameLift
            Gem::CertificateManager
            3rdParty::AWSNativeSDK::Core
)
ly_add_source_properties(
    SOURCES
        Source/BehaviorContext/GridSystemContext.cpp
        Source/MultiplayerEventsComponent.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_target(
    NAME Multiplayer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Multiplayer.d3ed407a19bb4c0d92b7c4872313d600.v1.0.0
    FILES_CMAKE
        multiplayer_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Multiplayer.Static
)

################################################################################
# Dedicated Server
################################################################################
if (PAL_TRAIT_BUILD_SERVER_SUPPORTED)

    ly_add_target(
        NAME Multiplayer.Server.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_source_dir}
                .
            PUBLIC
                Include
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        BUILD_DEPENDENCIES
            PUBLIC
                Legacy::CryCommon
                Legacy::CryNetwork.Static
                AZ::GridMate
                Gem::GameLift.Server
                Gem::CertificateManager
                3rdParty::AWSNativeSDK::Core
    )

    ly_add_target(
        NAME Multiplayer.Server MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Multiplayer.Server.d3ed407a19bb4c0d92b7c4872313d600.v1.0.0
        FILES_CMAKE
            multiplayer_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::Multiplayer.Server.Static
    )
endif ()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
         set(GAMELIFT_SERVER_TESTS_LIB  Gem::GameLift.Tests)
    endif()
    ly_add_target(
        NAME Multiplayer.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            multiplayer_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                ${pal_source_dir}
                Source
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::Multiplayer.Static
                ${GAMELIFT_SERVER_TESTS_LIB}
    )
    ly_add_googletest(
        NAME Gem::Multiplayer.Tests
    )
endif()
