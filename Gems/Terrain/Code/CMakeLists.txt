#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Terrain.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        terrain_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::Atom_Feature_Common
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::GradientSignal
            Gem::SurfaceData
            Gem::LmbrCentral
)

ly_add_target(
    NAME Terrain ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        terrain_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Terrain.Static
            Gem::LmbrCentral
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# the above module is for use in all client/server types
ly_create_alias(NAME Terrain.Servers  NAMESPACE Gem TARGETS Gem::Terrain Gem::SurfaceData.Servers Gem::GradientSignal.Servers)
ly_create_alias(NAME Terrain.Clients  NAMESPACE Gem TARGETS Gem::Terrain Gem::SurfaceData.Clients Gem::GradientSignal.Clients)

# If we are on a host platform, we want to add the host tools targets like the Terrain.Editor target which
# will also depend on Terrain.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Terrain.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            terrain_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                TERRAIN_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::GradientSignal
                Gem::LmbrCentral
                Gem::Terrain.Static
    )

    # the above module is for use in dev tool situations
    ly_create_alias(NAME Terrain.Builders NAMESPACE Gem TARGETS Gem::Terrain.Editor Gem::SurfaceData.Builders Gem::GradientSignal.Builders)
    ly_create_alias(NAME Terrain.Tools    NAMESPACE Gem TARGETS Gem::Terrain.Editor Gem::SurfaceData.Tools Gem::GradientSignal.Tools)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Terrain.Mocks HEADERONLY
        NAMESPACE Gem
        FILES_CMAKE
            terrain_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )
    ly_add_target(
        NAME Terrain.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            terrain_files.cmake
            terrain_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::LmbrCentral.Mocks
                Gem::GradientSignal.Mocks
                Gem::Terrain.Mocks
                Gem::Terrain.Static
    )

    # Add Terrain.Tests to googletest
    ly_add_googletest(
        NAME Gem::Terrain.Tests
    )

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We support Terrain.Editor.Tests on this platform, add Terrain.Editor.Tests target which depends on Terrain.Editor
        ly_add_target(
            NAME Terrain.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                terrain_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::LmbrCentral.Mocks
                    Gem::Terrain.Mocks
                    Gem::Terrain.Editor
        )

        # Add Terrain.Editor.Tests to googletest
        ly_add_googletest(
            NAME Gem::Terrain.Editor.Tests
        )
    endif()
endif()
