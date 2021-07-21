#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME GameState.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        gamestate_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME GameState ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        gamestate_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::GameState.Static
)

# Clients and Servers use the above module.  There is no editor or tools module required.
ly_create_alias(NAME GameState.Clients  NAMESPACE Gem TARGETS GameState)
ly_create_alias(NAME GameState.Servers  NAMESPACE Gem TARGETS GameState)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME GameState.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            gamestate_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::GameState.Static
    )
    ly_add_googletest(
        NAME Gem::GameState.Tests
    )
endif()
