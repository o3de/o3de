#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Turn off warnings for RecastNavigation::Detour library
set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_OPTIONS -Wno-everything
)
