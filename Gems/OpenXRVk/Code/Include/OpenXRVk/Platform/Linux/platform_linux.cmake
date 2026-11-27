#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB)
    set(LY_COMPILE_DEFINITIONS PUBLIC PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB)
endif()

if(PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND)
    set(LY_COMPILE_DEFINITIONS PUBLIC PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND)
endif()
