#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Gestures.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        gestures_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::Atom_RPI.Public
            AZ::AtomCore
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME Gestures ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        gestures_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::Gestures.Static
)

# All types of applications use the same module.
ly_create_alias(NAME Gestures.Clients  NAMESPACE Gem TARGETS Gestures)
ly_create_alias(NAME Gestures.Servers  NAMESPACE Gem TARGETS Gestures)
ly_create_alias(NAME Gestures.Builders NAMESPACE Gem TARGETS Gestures)
ly_create_alias(NAME Gestures.Tools    NAMESPACE Gem TARGETS Gestures)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Gestures.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            gestures_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::Gestures.Static
    )
    ly_add_googletest(
        NAME Gem::Gestures.Tests
    )
endif()
