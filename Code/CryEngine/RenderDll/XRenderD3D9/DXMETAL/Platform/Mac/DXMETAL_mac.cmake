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

set(LY_COMPILE_OPTIONS
    PRIVATE
        -xobjective-c++
)


find_library(METAL_LIBRARY Metal)
find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)
find_library(CORE_SERVICES_LIBRARY CoreServices)
find_library(QUARTZ_CORE_LIBRARY QuartzCore)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${METAL_LIBRARY}
        ${CORE_GRAPHICS_LIBRARY}
        ${CORE_SERVICES_LIBRARY}
        ${QUARTZ_CORE_LIBRARY}
)
