#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
