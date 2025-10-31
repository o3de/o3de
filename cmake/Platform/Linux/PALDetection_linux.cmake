#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_set(LY_PLATFORM_DETECTION_Linux Linux)
ly_set(LY_HOST_PLATFORM_DETECTION_Linux Linux)

# Linux supports multiple system architectures
ly_set(LY_ARCHITECTURE_DETECTION_Linux ${CMAKE_SYSTEM_PROCESSOR})
ly_set(LY_HOST_ARCHITECTURE_DETECTION_Linux ${CMAKE_HOST_SYSTEM_PROCESSOR})
