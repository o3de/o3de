#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Set resources directory for app icons
target_sources(Editor PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Images.xcassets)
set_target_properties(Editor PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/gui_info.plist
    RESOURCE ${CMAKE_CURRENT_LIST_DIR}/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME EditorAppIcon
)

# We cannot use ly_add_target here because we're already including this file from inside ly_add_target
# So we need to setup target, dependencies and install logic manually.
add_executable(EditorDummy Platform/Mac/main_dummy.cpp)
add_executable(AZ::EditorDummy ALIAS EditorDummy)

ly_target_link_libraries(EditorDummy
    PRIVATE
        AZ::AzCore
        AZ::AzFramework)

ly_add_dependencies(Editor EditorDummy)

# Store the aliased target into a DIRECTORY property
set_property(DIRECTORY APPEND PROPERTY LY_DIRECTORY_TARGETS AZ::EditorDummy)

# Store the directory path in a GLOBAL property so that it can be accessed
# in the layout install logic. Skip if the directory has already been added
get_property(ly_all_target_directories GLOBAL PROPERTY LY_ALL_TARGET_DIRECTORIES)
if(NOT CMAKE_CURRENT_SOURCE_DIR IN_LIST ly_all_target_directories)
    set_property(GLOBAL APPEND PROPERTY LY_ALL_TARGET_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})
endif()

ly_install_add_install_path_setreg(Editor)