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

set(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

include(${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_QTFORPYTHON_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME ${gem_name}.Editor.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        qtforpython_editor_files.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}/qtforpython_editor_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/qtforpython_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            3rdParty::Qt::Widgets
            Gem::EditorPythonBindings.Static
    RUNTIME_DEPENDENCIES
        3rdParty::pyside2
        3rdParty::Qt::Test
)

ly_add_target(
    NAME ${gem_name}.Editor GEM_MODULE
    NAMESPACE Gem
    FILES_CMAKE
        qtforpython_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Editor.Static
    RUNTIME_DEPENDENCIES
        3rdParty::pyside2::Tools
        Gem::EditorPythonBindings.Editor
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/QtForPythonModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# the above target is used in both builders like AssetProcessor and Tools like the Editor
# but is not used in clients or servers
ly_create_alias(NAME ${gem_name}.Tools     NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
ly_create_alias(NAME ${gem_name}.Builders  NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

