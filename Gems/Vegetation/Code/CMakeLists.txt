#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME Vegetation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        vegetation_files.cmake
    COMPILE_DEFINITIONS
        PUBLIC
            # if we are in debug or profile, enable VEG_PROFILE_ENABLED
            $<$<IN_LIST:$<CONFIG>,debug;profile>:VEG_PROFILE_ENABLED>
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LmbrCentral
            Gem::SurfaceData
        PUBLIC
            Gem::AtomLyIntegration_CommonFeatures.Static
    RUNTIME_DEPENDENCIES
        Gem::GradientSignal
)

ly_add_target(
    NAME Vegetation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        vegetation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Vegetation.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
        Gem::GradientSignal
        Gem::SurfaceData
)

# the above module is for use in clients and server type applications
ly_create_alias(NAME Vegetation.Servers NAMESPACE Gem TARGETS Gem::Vegetation)
ly_create_alias(NAME Vegetation.Clients NAMESPACE Gem TARGETS Gem::Vegetation)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Vegetation.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            vegetation_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                VEGETATION_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::Vegetation.Static
                AZ::AzToolsFramework
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral.Editor
        Gem::GradientSignal.Editor
        Gem::SurfaceData.Editor
    )
    # the above module is for use in dev tools
    ly_create_alias(NAME Vegetation.Builders NAMESPACE Gem TARGETS Gem::Vegetation.Editor)
    ly_create_alias(NAME Vegetation.Tools    NAMESPACE Gem TARGETS Gem::Vegetation.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Vegetation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            vegetation_tests_files.cmake
            vegetation_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFrameworkTestShared
                Gem::Vegetation.Static
                Gem::LmbrCentral.Mocks
    )
    ly_add_googletest(
        NAME Gem::Vegetation.Tests
    )
endif()
