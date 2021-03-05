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

set(LY_PLATFORM_DETECTION_Android Android)
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Darwin)
    set(LY_HOST_PLATFORM_DETECTION_Android Mac)
else()
    set(LY_HOST_PLATFORM_DETECTION_Android ${CMAKE_HOST_SYSTEM_NAME})
endif()
