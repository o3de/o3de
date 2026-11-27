#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_COMPILE_DEFINITIONS 
        PRIVATE
            AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH)

find_library(APPKIT_LIBRARY AppKit)
find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${APPKIT_LIBRARY}
        ${CORE_GRAPHICS_LIBRARY}
)
