#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(LY_PLATFORM_DETECTION_Android Android)
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Darwin)
    ly_set(LY_HOST_PLATFORM_DETECTION_Android Mac)
elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
    # Linux supports multiple system architectures
    ly_set(LY_HOST_PLATFORM_DETECTION_Android ${CMAKE_HOST_SYSTEM_NAME})
    ly_set(LY_HOST_ARCHITECTURE_DETECTION_Android ${CMAKE_HOST_SYSTEM_PROCESSOR})
else()
    ly_set(LY_HOST_PLATFORM_DETECTION_Android ${CMAKE_HOST_SYSTEM_NAME})
endif()
