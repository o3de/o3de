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
    ENTITLEMENT_FILE_PATH ${CMAKE_CURRENT_LIST_DIR}/EditorEntitlements.plist
)
