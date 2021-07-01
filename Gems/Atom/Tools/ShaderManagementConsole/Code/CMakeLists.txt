#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        Gem::Atom_RHI_DX12.Private
        Gem::Atom_RHI_Vulkan.Private
        Gem::Atom_RHI.Private
        Gem::Atom_RPI.Private
        Gem::Atom_RPI.Builders
        Gem::Atom_Feature_Common.Editor
        Gem::EditorPythonBindings.Editor
)

# Add build dependency to Editor for the ShaderManagementConsole application since
# Editor opens up the ShaderManagementConsole
ly_add_dependencies(Editor Gem::ShaderManagementConsole)

ly_add_target_dependencies(
    TARGETS
        ShaderManagementConsole
    DEPENDENCIES_FILES
        tool_dependencies.cmake
        Source/Platform/${PAL_PLATFORM_NAME}/tool_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
)

# Inject the project path into the ShaderManagementConsole VS debugger command arguments if the build system being invoked
# in a project centric view
if(NOT PROJECT_NAME STREQUAL "O3DE")
    set_property(TARGET ShaderManagementConsole APPEND PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--project-path=\"${CMAKE_SOURCE_DIR}\"")
endif()

# Adds the ShaderManagementConsole target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
set_source_files_properties(
    Source/ShaderManagementConsoleApplication.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
            LY_CMAKE_TARGET="ShaderManagementConsole"
)
