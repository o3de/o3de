#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Set resources directory for app icons
target_sources(AssetProcessor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/Images.xcassets)
set_target_properties(AssetProcessor PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/gui_info.plist
    RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AssetProcessorAppIcon
    ENTITLEMENT_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/AssetProcessorEntitlements.plist
)
