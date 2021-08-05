#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME MultiplayerCompression.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        multiplayercompression_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::lz4
            AZ::AzNetworking
            AZ::AzCore
)

ly_add_target(
    NAME MultiplayerCompression ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        multiplayercompression_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MultiplayerCompression.Static
)

# use the MultiplayerCompression module everywhere except builders:
ly_create_alias(NAME MultiplayerCompression.Clients NAMESPACE Gem TARGETS Gem::MultiplayerCompression)
ly_create_alias(NAME MultiplayerCompression.Tools   NAMESPACE Gem TARGETS Gem::MultiplayerCompression)
ly_create_alias(NAME MultiplayerCompression.Servers NAMESPACE Gem TARGETS Gem::MultiplayerCompression)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME MultiplayerCompression.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            multiplayercompression_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::MultiplayerCompression.Static
    )
    ly_add_googletest(
        NAME Gem::MultiplayerCompression.Tests
    )
endif()
