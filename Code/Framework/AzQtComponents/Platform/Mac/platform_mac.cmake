#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(FOUNDATION_LIBRARY Foundation)
find_library(OPENGL_LIBRARY OpenGL)
find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)
find_library(CORE_SERVICES_LIBRARY CoreServices)

set(LY_BUILD_DEPENDENCIES
    PUBLIC
        3rdParty::Qt::MacExtras
    PRIVATE
        ${FOUNDATION_LIBRARY}
        ${OPENGL_LIBRARY}
        ${CORE_SERVICES_LIBRARY}
        ${CORE_GRAPHICS_LIBRARY}
)
