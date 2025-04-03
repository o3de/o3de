#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        atomtoolsframework_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Python
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Network
            3rdParty::Qt::Widgets
            AZ::AzCore
            AZ::AzQtComponents
            AZ::AzToolsFramework
            Gem::Atom_Bootstrap.Headers
            Gem::Atom_Component_DebugCamera.Static
            Gem::Atom_Feature_Common
            Gem::Atom_Feature_Common.Public
            Gem::Atom_RHI.Public
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Edit
            Gem::Atom_RPI.Public
            Gem::GraphCanvasWidgets
            Gem::GraphModel.Editor.Static
            Gem::ImageProcessingAtom.Headers
)

ly_add_target(
    NAME ${gem_name}.Editor GEM_MODULE

    NAMESPACE Gem
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        atomtoolsframework_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_Bootstrap
        Gem::Atom_Component_DebugCamera
        Gem::Atom_Feature_Common.Editor
        Gem::Atom_RHI.Private
        Gem::Atom_RHI_Null.Private
        Gem::Atom_RPI.Editor
        Gem::EditorPythonBindings.Editor
        Gem::GraphCanvas.Editor
        Gem::GraphCanvasWidgets
        Gem::GraphModel.Editor
        Gem::ImageProcessingAtom.Editor
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AtomToolsFrameworkModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Add a 'builders' alias to allow the ${gem_name} root gem path to be added to the generated
# cmake_dependencies.<project>.assetprocessor.setreg to allow the asset scan folder for it to be added
ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atomtoolsframework_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Test
                AZ::AzFrameworkTestShared
                AZ::AzTest
                AZ::AzTestShared
                Gem::Atom_Utils.TestUtils.Static
                Gem::${gem_name}.Static
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

endif()
