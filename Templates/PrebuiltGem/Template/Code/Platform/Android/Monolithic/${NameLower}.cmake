#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_property(${NameLower}_gem_root GLOBAL PROPERTY "@GEMROOT:${Name}@")
list(APPEND LY_TARGET_PROPERTIES
    IMPORTED_LOCATION ${${NameLower}_gem_root}/lib/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/release/lib${Name}.a
    IMPORTED_LOCATION_DEBUG ${${NameLower}_gem_root}/lib/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/debug/lib${Name}.a
)
