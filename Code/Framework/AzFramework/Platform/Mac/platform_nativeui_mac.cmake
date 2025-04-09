#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(APPKIT_LIBRARY AppKit)
find_library(GAME_CONTROLLER_LIBRARY GameController)
find_library(CARBON_LIBRARY Carbon)
find_library(CORE_SERVICES_LIBRARY CoreServices)
find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${APPKIT_LIBRARY}
        ${GAME_CONTROLLER_LIBRARY}
        ${CARBON_LIBRARY}
        ${CORE_SERVICES_LIBRARY}
        ${CORE_GRAPHICS_LIBRARY}
)
