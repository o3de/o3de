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

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_MATERIAL_CANVAS_APPLICATION_SUPPORTED

if(NOT PAL_TRAIT_ATOM_MATERIAL_CANVAS_APPLICATION_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME MaterialCanvas APPLICATION
    NAMESPACE Gem
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        materialcanvas_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/tool_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${pal_source_dir}/shader_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::CommonFeaturesAtom.Static
            Gem::AtomToolsFramework.Editor
            Gem::AtomToolsFramework.Static
            # AzslCompiler and ShaderBuilderUtility for the in-memory preview shader path, shared with the Shader Asset Builder.
            Gem::AtomShader.Static
    RUNTIME_DEPENDENCIES
        Gem::AtomToolsFramework.Editor
)

if(PAL_TRAIT_BUILD_EXTERNAL_CRASH_HANDLER_SUPPORTED)
    ly_target_link_libraries(MaterialCanvas PRIVATE AZ::ToolsCrashHandler)
endif()

ly_set_gem_variant_to_load(TARGETS MaterialCanvas VARIANTS Tools)

# Add a 'builders' alias to allow the MaterialCanvas root gem path to be added to the generated
# cmake_dependencies.<project>.assetprocessor.setreg to allow the asset scan folder for it to be added
ly_create_alias(NAME MaterialCanvas.Builders NAMESPACE Gem)

# Add build dependency to Editor for the MaterialCanvas application since
# Editor opens up the MaterialCanvas
ly_add_dependencies(Editor Gem::MaterialCanvas)

# Inject the project path into the MaterialCanvas VS debugger command arguments if the build system being invoked
# in a project centric view
if(NOT PROJECT_NAME STREQUAL "O3DE")
    set_property(TARGET MaterialCanvas APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${CMAKE_SOURCE_DIR}\"")
endif()

# Adds the MaterialCanvas target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET MaterialCanvas)
    set_source_files_properties(
        Source/MaterialCanvasApplication.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="MaterialCanvas"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to MaterialCanvas as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()

# Editor-hosted Material Canvas pane: a STATIC target plus a thin GEM_MODULE the Editor loads, as in Gems/LandscapeCanvas.

ly_add_target(
    NAME MaterialCanvas.Editor.Static STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTORCC
    FILES_CMAKE
        materialcanvas_editor_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/shader_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            # MaterialCanvas_Traits_Platform.h, which says whether the in-memory preview shader path exists on this platform.
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzToolsFramework
            # Provides LyViewPaneNames.h (LyViewPane::CategoryTools), as Landscape Canvas does.
            Legacy::Editor.Headers
        PUBLIC
            Gem::CommonFeaturesAtom.Static
            Gem::AtomToolsFramework.Static
            # See the note on the MaterialCanvas application target above.
            Gem::AtomShader.Static
)

ly_add_target(
    NAME MaterialCanvas.Editor GEM_MODULE
    NAMESPACE Gem
    AUTOMOC
    FILES_CMAKE
        materialcanvas_editor_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MaterialCanvas.Editor.Static
    RUNTIME_DEPENDENCIES
        Gem::AtomToolsFramework.Editor
)

# Inject the gem name into the module source file so AZ_DECLARE_MODULE_CLASS resolves to Gem_MaterialCanvas.
ly_add_source_properties(
    SOURCES
        Source/Editor/MaterialCanvasEditorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# The Editor loads the Tools variant; the standalone app loads it too, harmlessly, since nothing is built until NotifyRegisterViews.
ly_create_alias(NAME MaterialCanvas.Tools NAMESPACE Gem TARGETS Gem::MaterialCanvas.Editor)
