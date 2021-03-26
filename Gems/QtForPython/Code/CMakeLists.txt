#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        qtforpython_editor_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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
)

ly_add_target(
    NAME QtForPython.Editor MODULE 
    NAMESPACE Gem
    FILES_CMAKE
        qtforpython_shared_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::QtForPython.Editor.Static
    RUNTIME_DEPENDENCIES
        Gem::EditorPythonBindings.Editor
)
