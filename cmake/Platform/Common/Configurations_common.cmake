#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# All platforms and configurations end up including this file
# Here we clean up the variables and add common compilation flags to all platforms. Each platform then will add their
# own definitions. Common configurations can be shared by moving those configurations to the "Common" folder and making
# each platform include them

# Clear all
ly_set(CMAKE_C_FLAGS "")
ly_set(CMAKE_CXX_FLAGS "")
ly_set(LINK_OPTIONS "")
foreach(conf ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${conf} UCONF)
    ly_set(CMAKE_C_FLAGS_${UCONF} "")
    ly_set(CMAKE_CXX_FLAGS_${UCONF} "")
    ly_set(LINK_OPTIONS_${UCONF} "")
    ly_set(LY_BUILD_CONFIGURATION_TYPE_${UCONF} ${conf})
endforeach()

# Common configurations
ly_append_configurations_options(
    DEFINES
        # Since we disable exceptions, we need to define _HAS_EXCEPTIONS=0 so the STD does not add exception handling 
        _HAS_EXCEPTIONS=0
    DEFINES_DEBUG
        _DEBUG            # TODO: this should be able to removed since it gets added automatically by some compilation flags
        AZ_DEBUG_BUILD=1
        AZ_ENABLE_TRACING
        AZ_ENABLE_DEBUG_TOOLS
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_DEBUG}"
    DEFINES_PROFILE
        _PROFILE
        PROFILE
        NDEBUG
        AZ_ENABLE_TRACING
        AZ_ENABLE_DEBUG_TOOLS
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_PROFILE}"
    DEFINES_RELEASE
        _RELEASE
        RELEASE
        NDEBUG
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_RELEASE}"
)
