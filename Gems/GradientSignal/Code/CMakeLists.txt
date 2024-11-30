#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name}.Static STATIC
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
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::${gem_name}.Static
            Gem::ImageProcessingAtom.Headers # Atom/ImageProcessing/PixelFormats.h is part of a header in Includes
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/GradientSignalModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Load the "Gem::${gem_name}" module in Clients and Servers
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
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
                Gem::${gem_name}.Static
                Gem::SurfaceData
                Gem::LmbrCentral.Editor.Static
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
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
                Gem::${gem_name}.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/GradientSignalEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # Load the "Gem::${gem_name}.Editor" module in Builders and Tools
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Mocks HEADERONLY
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )
    
    ly_add_target(
        NAME ${gem_name}.Tests.Static STATIC
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
                Gem::Atom_RPI.TestUtils
                Gem::${gem_name}.Static
                Gem::LmbrCentral
                Gem::LmbrCentral.Mocks
                Gem::${gem_name}.Mocks
        RUNTIME_DEPENDENCIES
            Gem::SurfaceData
    )
    
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                Gem::${gem_name}.Tests.Static
                Gem::${gem_name}.Static
                Gem::LmbrCentral
                Gem::LmbrCentral.Mocks
                Gem::${gem_name}.Mocks
        RUNTIME_DEPENDENCIES
            Gem::SurfaceData
    )
    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_googlebenchmark(
        NAME Gem::${gem_name}.Benchmarks
        TARGET Gem::${gem_name}.Tests
    )


    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::${gem_name}.Tests.Static
                    Gem::${gem_name}.Static
                    Gem::${gem_name}.Editor.Static
                    Gem::LmbrCentral.Editor
                    Gem::LmbrCentral.Mocks
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()

endif()
