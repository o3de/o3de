#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(APPKIT_LIBRARY AppKit)
find_library(FOUNDATION_LIBRARY Foundation)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${APPKIT_LIBRARY}
        ${FOUNDATION_LIBRARY}
)
