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

ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

include(${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_QTFORPYTHON_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME QtForPython.Editor.Static STATIC
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
    NAME QtForPython.Editor GEM_MODULE
    NAMESPACE Gem
    FILES_CMAKE
        qtforpython_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/${PAL_PLATFORM_NAME}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::QtForPython.Editor.Static
    RUNTIME_DEPENDENCIES
        Gem::EditorPythonBindings.Editor
)

# the above target is used in both builders like AssetProcessor and Tools like the Editor
# but is not used in clients or servers
ly_create_alias(NAME QtForPython.Tools     NAMESPACE Gem TARGETS Gem::QtForPython.Editor)
ly_create_alias(NAME QtForPython.Builders  NAMESPACE Gem TARGETS Gem::QtForPython.Editor)

