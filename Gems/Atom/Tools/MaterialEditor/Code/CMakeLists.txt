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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_MATERIAL_EDITOR_APPLICATION_SUPPORTED

if(NOT PAL_TRAIT_ATOM_MATERIAL_EDITOR_APPLICATION_SUPPORTED)
    return()
endif()


ly_add_target(
    NAME MaterialEditor.Document STATIC
    NAMESPACE Gem
    AUTOMOC
    FILES_CMAKE
        materialeditordocument_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RPI.Edit
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
)

ly_add_target(
    NAME MaterialEditor.Window STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        materialeditorwindow_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Public
            Gem::ImageProcessingAtom.Headers
)

ly_add_target(
    NAME MaterialEditor.Viewport STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
   FILES_CMAKE
        materialeditorviewport_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        Public
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RHI.Public
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Component_DebugCamera.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
)

ly_add_target(
    NAME MaterialEditor EXECUTABLE
    NAMESPACE Gem
    AUTOMOC
    FILES_CMAKE
        materialeditor_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/tool_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::MaterialEditor.Window
            Gem::MaterialEditor.Viewport
            Gem::MaterialEditor.Document
    RUNTIME_DEPENDENCIES
        Gem::AtomToolsFramework.Editor
        Gem::EditorPythonBindings.Editor
        Gem::ImageProcessingAtom.Editor
)

ly_set_gem_variant_to_load(TARGETS MaterialEditor VARIANTS Tools)

# Add a 'builders' alias to allow the MaterialEditor root gem path to be added to the generated
# cmake_dependencies.<project>.assetprocessor.setreg to allow the asset scan folder for it to be added
ly_create_alias(NAME MaterialEditor.Builders NAMESPACE Gem)

# Add build dependency to Editor for the MaterialEditor application since
# Editor opens up the MaterialEditor
ly_add_dependencies(Editor Gem::MaterialEditor)

# Inject the project path into the MaterialEditor VS debugger command arguments if the build system being invoked
# in a project centric view
if(NOT PROJECT_NAME STREQUAL "O3DE")
    set_property(TARGET MaterialEditor APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${CMAKE_SOURCE_DIR}\"")
endif()

# Adds the MaterialEditor target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET MaterialEditor)
    set_source_files_properties(
        Source/MaterialEditorApplication.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="MaterialEditor"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to MaterialEditor as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()
