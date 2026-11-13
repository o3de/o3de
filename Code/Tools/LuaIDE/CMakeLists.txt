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

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

ly_add_target(
    NAME LuaIDE APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        lua_ide_files.cmake
        ${pal_dir}/lua_ide_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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
            AZ::AzNetworking
            AZ::AzQtComponents
            ${additional_dependencies}
    COMPILE_DEFINITIONS 
        PRIVATE
            STANDALONETOOLS_ENABLE_LUA_IDE
)

if(PAL_TRAIT_BUILD_EXTERNAL_CRASH_HANDLER_SUPPORTED)
    ly_target_link_libraries(LuaIDE PRIVATE AZ::ToolsCrashHandler)
endif()

ly_set_gem_variant_to_load(TARGETS LuaIDE VARIANTS LuaTools)
ly_add_dependencies(Editor LuaIDE)

################################################################################
# Tests
#
# The flattened GUI's elements (Source/FlatElements.cpp) have a test target of their own: the application's other sources are not
# built into it, so what the twins reach from the code view and the document gutter is a stand-in of the same calls.
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME LuaIDE.Tests EXECUTABLE
        NAMESPACE AZ
        FILES_CMAKE
            lua_ide_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzQtComponents
                AZ::AzTest
    )

    ly_add_googletest(
        NAME AZ::LuaIDE.Tests
        LABELS REQUIRES_tiaf
        TEST_COMMAND $<TARGET_FILE:AZ::LuaIDE.Tests> AzRunUnitTests
    )

endif()
