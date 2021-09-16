#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_executable(O3DE_SDK Platform/Mac/O3DE_SDK_Launcher.cpp)

set_target_properties(O3DE_SDK PROPERTIES
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS NO
)

ly_target_link_libraries(O3DE_SDK
    PRIVATE
        AZ::AzCore
        AZ::AzFramework
)
