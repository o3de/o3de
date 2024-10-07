#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_LINK_OPTIONS
    PRIVATE
        -ObjC
)

# Add resources and app icons to launchers
list(APPEND candidate_paths ${project_real_path}/Resources/Platform/iOS)
list(APPEND candidate_paths ${project_real_path}/Gem/Resources/Platform/iOS) # Legacy projects
list(APPEND candidate_paths ${project_real_path}/Gem/Resources/IOSLauncher) # Legacy projects
foreach(resource_path IN LISTS candidate_paths)
    if(EXISTS ${resource_path})
        set(ly_game_resource_folder ${resource_path})
        break()
    endif()
endforeach()

if(NOT EXISTS ${ly_game_resource_folder})
    list(JOIN candidate_paths " " formatted_error)
    message(FATAL_ERROR "Missing 'Resources' folder. Candidate paths tried were: ${formatted_error}")
endif()


target_sources(${project_name}.GameLauncher PRIVATE ${ly_game_resource_folder}/Images.xcassets)
set_target_properties(${project_name}.GameLauncher PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${ly_game_resource_folder}/Info.plist
    RESOURCE ${ly_game_resource_folder}/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ${project_name}AppIcon
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME LaunchImage
)

set(layout_tool_dir ${LY_ROOT_FOLDER}/cmake/Tools)

add_custom_command(TARGET ${project_name}.GameLauncher POST_BUILD
    COMMAND ${LY_PYTHON_CMD} layout_tool.py
        -p iOS
        -a ${LY_ASSET_DEPLOY_ASSET_TYPE}
        --project-path ${project_real_path}
        -m ${LY_ASSET_DEPLOY_MODE}
        --create-layout-root
        -l $<TARGET_BUNDLE_DIR:${project_name}.GameLauncher>/assets
        --build-config $<CONFIG>
        --warn-on-missing-assets
        --verify
        --copy
        --override-pak-folder ${project_real_path}/AssetBundling/Bundles
        ${LY_OVERRIDE_PAK_ARGUMENT}
    WORKING_DIRECTORY ${layout_tool_dir}
    COMMENT "Synchronizing Layout Assets ..."
    VERBATIM
)
