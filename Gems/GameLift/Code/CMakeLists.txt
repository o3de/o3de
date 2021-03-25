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
include(${pal_source_dir}/PAL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

ly_add_target(
    NAME GameLift.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        gridmate_gamelift_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Legacy::CryCommon
            3rdParty::OpenSSL
            3rdParty::AWSNativeSDK::GameLiftClient
        PUBLIC
            AZ::GridMate #GameLift client buses interface uses GridMate classes as function parameters
    COMPILE_DEFINITIONS
        PUBLIC
            # Disabling client and server gamelift integration so that we can redcode crynetwork
            # The new gamelift gem will work directly against the new transport framework and multiplayer gem
            #BUILD_GAMELIFT_CLIENT
)

ly_add_target( 
    NAME GameLift ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.GameLift.76de765796504906b73be7365a9bff06.v2.0.0
    FILES_CMAKE
        GameLift_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            3rdParty::AWSNativeSDK::Core
            AWSNativeSDKInit
            Gem::GameLift.Static
    COMPILE_DEFINITIONS
        PUBLIC
            # Disabling client and server gamelift integration so that we can redcode crynetwork
            # The new gamelift gem will work directly against the new transport framework and multiplayer gem
            #BUILD_GAMELIFT_CLIENT
)

################################################################################
# Dedicated Server
################################################################################
if (PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    # GameLift gem code consists of both server and client code separated using defines. The dependencies are same only separated by define.
    # TODO: With cmake since we are moving to create targets for the server code, the defines to segregate client and server would become obsolete.
    # The static project should be the common project between client and server and contain the common code like replicated objets.

    ly_add_target(
        NAME GameLift.Server.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            gridmate_gamelift_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore            
                3rdParty::OpenSSL
            PUBLIC
                AZ::GridMate #GameLift server buses interface uses GridMate classes as function parameters
                3rdParty::AWSGameLiftServerSDK
        COMPILE_DEFINITIONS
            PUBLIC
                # Disabling client and server gamelift integration so that we can redcode crynetwork
                # The new gamelift gem will work directly against the new transport framework and multiplayer gem
                #BUILD_GAMELIFT_SERVER
)

    ly_add_target(
        NAME GameLift.Server MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.GameLift.Server.76de765796504906b73be7365a9bff06.v2.0.0
        FILES_CMAKE
            GameLift_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                3rdParty::AWSNativeSDK::Core
                AWSNativeSDKInit
                GameLift.Server.Static
            PUBLIC
                3rdParty::AWSGameLiftServerSDK #GameLiftServerService.h includes GameLiftServiceEventsBus.h which include GameLiftServerAPI
        COMPILE_DEFINITIONS
            PUBLIC
                # Disabling client and server gamelift integration so that we can redcode crynetwork
                # The new gamelift gem will work directly against the new transport framework and multiplayer gem
                #BUILD_GAMELIFT_SERVER
    )
endif ()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_ENABLE_GAMELIFT_UNIT_TEST)
    if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
        set(GAMELIFT_SERVER_TESTS_LIB Gem::GameLift.Server.Static)
    endif()
    ly_add_target(
        NAME GameLift.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            gamelift_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::GameLift.Static
                3rdParty::AWSNativeSDK::GameLiftClient
            PUBLIC
                ${GAMELIFT_SERVER_TESTS_LIB}
   )
   ly_add_googletest(
        NAME Gem::GameLift.Tests
    )
endif()
