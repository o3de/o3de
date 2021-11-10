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

ly_add_target(
    NAME LuaIDE APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        standalone_tools_files.cmake
        lua_ide_files.cmake
        Platform/${PAL_PLATFORM_NAME}/lua_ide_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source/Editor
            Source/LUA
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::GridMate
            AZ::AzQtComponents
            ${additional_dependencies}
    COMPILE_DEFINITIONS 
        PRIVATE
            STANDALONETOOLS_ENABLE_LUA_IDE
)

ly_add_dependencies(Editor LuaIDE)
