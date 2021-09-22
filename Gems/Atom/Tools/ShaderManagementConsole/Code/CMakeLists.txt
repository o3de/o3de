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

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_SHADER_MANAGEMENT_CONSOLE_APPLICATION_SUPPORTED

if(NOT PAL_TRAIT_ATOM_SHADER_MANAGEMENT_CONSOLE_APPLICATION_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME ShaderManagementConsole.Document STATIC
    NAMESPACE Gem
    FILES_CMAKE
        shadermanagementconsoledocument_files.cmake
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
    NAME ShaderManagementConsole.Window STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        shadermanagementconsolewindow_files.cmake
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
)

ly_add_target(
    NAME ShaderManagementConsole EXECUTABLE
    NAMESPACE Gem
    FILES_CMAKE
        shadermanagementconsole_files.cmake
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
            Gem::ShaderManagementConsole.Window
            Gem::ShaderManagementConsole.Document
    RUNTIME_DEPENDENCIES
        Gem::AtomToolsFramework.Editor
        Gem::EditorPythonBindings.Editor
)

ly_set_gem_variant_to_load(TARGETS ShaderManagementConsole VARIANTS Tools)

# Add build dependency to Editor for the ShaderManagementConsole application since
# Editor opens up the ShaderManagementConsole
ly_add_dependencies(Editor Gem::ShaderManagementConsole)

# Inject the project path into the ShaderManagementConsole VS debugger command arguments if the build system being invoked
# in a project centric view
if(NOT PROJECT_NAME STREQUAL "O3DE")
    set_property(TARGET ShaderManagementConsole APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${CMAKE_SOURCE_DIR}\"")
endif()

# Adds the ShaderManagementConsole target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET ShaderManagementConsole)
    set_source_files_properties(
        Source/ShaderManagementConsoleApplication.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="ShaderManagementConsole"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to ShaderManagementConsole as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()
