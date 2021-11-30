#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(__azcore_dependencies)

find_library(FOUNDATION_LIBRARY Foundation)
list(APPEND __azcore_dependencies ${FOUNDATION_LIBRARY})


if (NOT LY_MONOLITHIC_GAME)
    find_library(UI_KIT_FRAMEWORK UIKit)
    list(APPEND __azcore_dependencies ${UI_KIT_FRAMEWORK})
endif()



set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${__azcore_dependencies}
)
