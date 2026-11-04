#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(IOKIT_LIBRARY IOKit)
find_library(APPKIT_LIBRARY AppKit)
find_library(COREVIDEO_FRAMEWORK CoreVideo)

find_package(OpenGL REQUIRED)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${IOKIT_LIBRARY}
        ${OpenGL_LIBRARIES}
        ${APPKIT_LIBRARY}
)
