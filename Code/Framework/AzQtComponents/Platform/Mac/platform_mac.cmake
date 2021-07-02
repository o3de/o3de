#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(FOUNDATION_LIBRARY Foundation)
find_library(OPENGL_LIBRARY OpenGL)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${FOUNDATION_LIBRARY}
        ${OPENGL_LIBRARY}
)

set(LY_BUILD_DEPENDENCIES
    PUBLIC
        3rdParty::Qt::MacExtras
)
