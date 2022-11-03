#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME FastNoise.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        fastnoise_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
            .       # To give acess to External/FastNoise
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::GradientSignal
        PUBLIC
            AZ::AzCore
        PRIVATE
            AZ::AzFramework
            Gem::LmbrCentral
)

ly_add_target(
    NAME FastNoise ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        fastnoise_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::FastNoise.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
        Gem::GradientSignal
)

# Clients and Servers use the above module
ly_create_alias(NAME FastNoise.Clients  NAMESPACE Gem TARGETS FastNoise)
ly_create_alias(NAME FastNoise.Servers  NAMESPACE Gem TARGETS FastNoise)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME FastNoise.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            fastnoise_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                FASTNOISE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::LmbrCentral.Editor
                Gem::GradientSignal.Editor.Static
            PUBLIC
                Gem::FastNoise.Static
                AZ::AzToolsFramework
                Gem::SurfaceData
    )

    ly_add_target(
        NAME FastNoise.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            fastnoise_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::FastNoise.Editor.Static
                Gem::LmbrCentral.Editor
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
            Gem::SurfaceData.Editor
    )

    # builders and tools load the above tool module.
    ly_create_alias(NAME FastNoise.Builders NAMESPACE Gem TARGETS FastNoise.Editor)
    ly_create_alias(NAME FastNoise.Tools    NAMESPACE Gem TARGETS FastNoise.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME FastNoise.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                fastnoise_editor_tests_files.cmake
            COMPILE_DEFINITIONS
                PUBLIC
                    FASTNOISE_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    FastNoise.Editor.Static
                    Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::FastNoise.Editor.Tests
        )
    endif()

    ly_add_target(
        NAME FastNoise.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            fastnoise_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::FastNoise.Static
                Gem::GradientSignal
                Gem::GradientSignal.Tests.Static
                Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Gem::FastNoise.Tests
    )

    ly_add_googlebenchmark(
        NAME Gem::FastNoise.Benchmarks
        TARGET Gem::FastNoise.Tests
    )

endif()
