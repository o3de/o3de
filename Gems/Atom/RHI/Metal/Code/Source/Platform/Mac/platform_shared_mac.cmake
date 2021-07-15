#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# These libraries are frameworks, so we use find_library's framework detection
# to find them for us
find_library(METAL_LIBRARY Metal)
find_library(COREGRAPHICS_LIBRARY CoreGraphics)
find_library(CORESERVICES_LIBRARY CoreServices)
find_library(QUARTZCORE_LIBRARY QuartzCore)
mark_as_advanced(METAL_LIBRARY COREGRAPHICS_LIBRARY CORESERVICES_LIBRARY QUARTZCORE_LIBRARY)

set(LY_BUILD_DEPENDENCIES
    PUBLIC
        ${METAL_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${CORESERVICES_LIBRARY}
        ${QUARTZCORE_LIBRARY}
)
