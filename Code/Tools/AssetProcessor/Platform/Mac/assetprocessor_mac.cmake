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

ly_add_bundle_resources(
    TARGET AssetProcessor
    FILES
        ${CMAKE_SOURCE_DIR}/Code/Tools/RC/Config/rc/xmlfilter.txt
        ${CMAKE_SOURCE_DIR}/Code/Tools/RC/Config/rc/rc.ini
)

# Set resources directory for app icons
target_sources(AssetProcessor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/Images.xcassets)
set_target_properties(AssetProcessor PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/gui_info.plist
    RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Mac/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AssetProcessorAppIcon
)
