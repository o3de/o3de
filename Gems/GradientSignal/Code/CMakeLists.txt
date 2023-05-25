#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME GradientSignal.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        gradientsignal_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::SurfaceData
            Gem::ImageProcessingAtom.Headers
            Gem::LmbrCentral
)

ly_add_target(
    NAME GradientSignal ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        gradientsignal_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LmbrCentral.API
        PUBLIC
            AZ::AzCore
            Gem::GradientSignal.Static
            Gem::ImageProcessingAtom.Headers # Atom/ImageProcessing/PixelFormats.h is part of a header in Includes
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Load the "Gem::GradientSignal" module in Clients and Servers
ly_create_alias(NAME GradientSignal.Clients NAMESPACE Gem TARGETS Gem::GradientSignal)
ly_create_alias(NAME GradientSignal.Servers NAMESPACE Gem TARGETS Gem::GradientSignal)
ly_create_alias(NAME GradientSignal.Unified NAMESPACE Gem TARGETS Gem::GradientSignal)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME GradientSignal.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            gradientsignal_editor_files.cmake
        PLATFORM_INCLUDE_FILES
            Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/gradient_signal_editor_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PUBLIC
                GRADIENTSIGNAL_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::OpenImageIO
                Gem::LmbrCentral.Editor
                Gem::Atom_RPI.Edit
            PUBLIC
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Gem::GradientSignal.Static
                Gem::SurfaceData
                Gem::LmbrCentral.Editor.Static
    )

    ly_add_target(
        NAME GradientSignal.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::GradientSignal.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    # Load the "Gem::GradientSignal.Editor" module in Builders and Tools
    ly_create_alias(NAME GradientSignal.Builders NAMESPACE Gem TARGETS Gem::GradientSignal.Editor)
    ly_create_alias(NAME GradientSignal.Tools    NAMESPACE Gem TARGETS Gem::GradientSignal.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME GradientSignal.Mocks HEADERONLY
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )
    
    ly_add_target(
        NAME GradientSignal.Tests.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_shared_tests_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Tests
            PRIVATE
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzTestShared
                Gem::Atom_RPI.TestUtils.Static
                Gem::GradientSignal.Static
                Gem::LmbrCentral
                Gem::LmbrCentral.Mocks
                Gem::GradientSignal.Mocks
        RUNTIME_DEPENDENCIES
            Gem::SurfaceData
    )
    
    ly_add_target(
        NAME GradientSignal.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzTestShared
                Gem::GradientSignal.Tests.Static
                Gem::GradientSignal.Static
                Gem::LmbrCentral
                Gem::LmbrCentral.Mocks
                Gem::GradientSignal.Mocks
        RUNTIME_DEPENDENCIES
            Gem::SurfaceData
    )
    ly_add_googletest(
        NAME Gem::GradientSignal.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_googlebenchmark(
        NAME Gem::GradientSignal.Benchmarks
        TARGET Gem::GradientSignal.Tests
    )


    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME GradientSignal.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                gradientsignal_editor_tests_files.cmake
            PLATFORM_INCLUDE_FILES
                Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/gradient_signal_editor_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::OpenImageIO
                    AZ::AzTest
                    AZ::AzTestShared
                    Gem::GradientSignal.Tests.Static
                    Gem::GradientSignal.Static
                    Gem::GradientSignal.Editor.Static
                    Gem::LmbrCentral.Editor
                    Gem::LmbrCentral.Mocks
        )
        ly_add_googletest(
            NAME Gem::GradientSignal.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()

endif()
