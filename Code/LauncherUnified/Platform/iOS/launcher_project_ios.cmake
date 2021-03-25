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

set(LY_LINK_OPTIONS
    PRIVATE
        -ObjC
)

if(LY_MONOLITHIC_GAME) # only Atom is supported in monolithic
    list(APPEND LY_BUILD_DEPENDENCIES Legacy::CryRenderOther)
else()
    list(APPEND LY_BUILD_DEPENDENCIES CrySystem.Static)
    set(LY_RUNTIME_DEPENDENCIES Legacy::CryRenderMetal)
endif()

# Find the resource from the game gem
get_target_property(${project}_SOURCE_DIR ${project} SOURCE_DIR) # Point to where the code is
get_filename_component(${project}_SOURCE_PARENT_DIR ${${project}_SOURCE_DIR} DIRECTORY) # Parent directory

set(ly_game_resource_folder ${${project}_SOURCE_PARENT_DIR}/Resources/Platform/iOS)
if (NOT EXISTS ${ly_game_resource_folder})
    set(ly_game_resource_folder ${${project}_SOURCE_PARENT_DIR}/Resources/IOSLauncher)
    if (NOT EXISTS ${ly_game_resource_folder})
        message(FATAL_ERROR "Missing expected resources folder")
    endif()
endif()

# Add resources and app icons to launchers
target_sources(${project}.GameLauncher PRIVATE ${ly_game_resource_folder}/Images.xcassets)
set_target_properties(${project}.GameLauncher PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${ly_game_resource_folder}/Info.plist
    RESOURCE ${ly_game_resource_folder}/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ${project}AppIcon
)

set(layout_tool_dir ${LY_ROOT_FOLDER}/cmake/Tools)

add_custom_command(TARGET ${project}.GameLauncher POST_BUILD
    COMMAND ${LY_PYTHON_CMD} layout_tool.py
        --dev-root "${LY_ROOT_FOLDER}"
        -p iOS
        -a ${LY_ASSET_DEPLOY_ASSET_TYPE}
        -g ${project}
        -m ${LY_ASSET_DEPLOY_MODE}
        --create-layout-root
        -l $<TARGET_BUNDLE_DIR:${project}.GameLauncher>/assets
        --build-config $<CONFIG>
        --warn-on-missing-assets
        --verify
        --copy
        ${LY_OVERRIDE_PAK_ARGUMENT}
    WORKING_DIRECTORY ${layout_tool_dir}
    COMMENT "Synchronizing Layout Assets ..."
    VERBATIM
)