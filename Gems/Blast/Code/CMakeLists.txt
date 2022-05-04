#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_BLAST Traits

if(NOT PAL_TRAIT_BLAST_SUPPORTED)
    include(blast_unsupported.cmake)
    return()
endif()

ly_add_target(
    NAME Blast.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        blast_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            .
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Blast
            3rdParty::PhysX
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_Feature_Common.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::PhysX.Static
)

ly_add_target(
    NAME Blast ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Blast.Gem
    FILES_CMAKE
        blast_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            .
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Blast.Static
    RUNTIME_DEPENDENCIES
        Gem::PhysX
)

# clients and servers use the above Gem module.
ly_create_alias(NAME Blast.Servers NAMESPACE Gem TARGETS Gem::Blast)
ly_create_alias(NAME Blast.Clients NAMESPACE Gem TARGETS Gem::Blast)


if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Blast.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            blast_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                BLAST_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                AZ::GFxFramework
                AZ::SceneCore
                AZ::SceneData
                Legacy::Editor.Headers
                Gem::Atom_RPI.Edit
                Gem::Blast.Static
    )

    ly_add_target(
        NAME Blast.Editor GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME Blast.Editor.Gem
        AUTOMOC
        FILES_CMAKE
            blast_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Blast.Editor.Static
                Gem::PythonAssetBuilder.Editor
        RUNTIME_DEPENDENCIES
            3rdParty::assimplib
            Gem::PhysX.Editor
    )

    # tools and builders use the above module.
    ly_create_alias(NAME Blast.Tools NAMESPACE    Gem TARGETS Gem::Blast.Editor)
    ly_create_alias(NAME Blast.Builders NAMESPACE Gem TARGETS Gem::Blast.Editor)
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Blast.Tests MODULE
        NAMESPACE Gem
        OUTPUT_NAME Blast.Tests.Gem
        FILES_CMAKE
            blast_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::Atom_Feature_Common.Tests
                Gem::Blast.Static
    )
    ly_add_googletest(
        NAME Gem::Blast.Tests
        TEST_SUITE sandbox
    )
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME Blast.Editor.Tests MODULE
            NAMESPACE Gem
            OUTPUT_NAME Blast.Editor.Tests.Gem
            FILES_CMAKE
                blast_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Test
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzCoreTestCommon
                    AZ::AzToolsFrameworkTestCommon
                    Gem::Blast.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::Blast.Editor.Tests
        )
    endif()
endif()
