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

ly_add_target(
    NAME LuaIDE APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        woodpecker_files.cmake
        lua_ide_files.cmake
        Platform/${PAL_PLATFORM_NAME}/lua_ide_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Woodpecker/Editor
            Woodpecker/LUA
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
            UNICODE
            WOODPECKER_ENABLE_LUA_IDE
)

ly_add_target(
    NAME Profiler APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        woodpecker_files.cmake
        profiler_files.cmake
        Platform/${PAL_PLATFORM_NAME}/profiler_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Woodpecker/Editor
            Woodpecker/Driller
            Woodpecker/Driller/Carrier
            Woodpecker/Driller/Profiler
            Woodpecker/Driller/IO
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::GridMate
            ${additional_dependencies}
    COMPILE_DEFINITIONS 
        PRIVATE
            UNICODE
            WOODPECKER_ENABLE_PROFILER
)
