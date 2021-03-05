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

find_library(GAME_CONTROLLER_FRAMEWORK GameController)
find_library(UI_KIT_FRAMEWORK UIKit)
find_library(CORE_MOTION_FRAMEWORK CoreMotion)
find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics)


set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${GAME_CONTROLLER_FRAMEWORK}
        ${UI_KIT_FRAMEWORK}
        ${CORE_MOTION_FRAMEWORK}
        ${CORE_GRAPHICS_FRAMEWORK}
)
