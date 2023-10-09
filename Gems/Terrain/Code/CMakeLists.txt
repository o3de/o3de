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
            Gem::Atom_Feature_Common.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::GradientSignal.Static
            Gem::SurfaceData.Static
            Gem::LmbrCentral.API
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
            Gem::LmbrCentral.API
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# the above module is for use in all client/server types
ly_create_alias(NAME Terrain.Servers  NAMESPACE Gem TARGETS Gem::Terrain Gem::SurfaceData.Servers Gem::GradientSignal.Servers)
ly_create_alias(NAME Terrain.Clients  NAMESPACE Gem TARGETS Gem::Terrain Gem::SurfaceData.Clients Gem::GradientSignal.Clients)
ly_create_alias(NAME Terrain.Unified  NAMESPACE Gem TARGETS Gem::Terrain Gem::SurfaceData.Unified Gem::GradientSignal.Unified)

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
            PRIVATE
                Gem::Atom_RPI.Edit
                Gem::GradientSignal.Editor.Static
            PUBLIC
                AZ::AzToolsFramework
                Gem::GradientSignal.Static
                Gem::LmbrCentral.API
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
            terrain_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFrameworkTestShared
                AZ::AzTestShared
                AZ::AzFramework
                Gem::Atom_RPI.TestUtils.Static
                Gem::GradientSignal.Tests.Static
                Gem::LmbrCentral.Mocks
                Gem::GradientSignal.Mocks
                Gem::Terrain.Mocks
                Gem::Terrain.Static
        RUNTIME_DEPENDENCIES
            Gem::GradientSignal
            Gem::LmbrCentral
            Gem::SurfaceData
    )

    # Add Terrain.Tests to googletest
    ly_add_googletest(
        NAME Gem::Terrain.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_googlebenchmark(
        NAME Gem::Terrain.Benchmarks
        TARGET Gem::Terrain.Tests
    )

endif()
