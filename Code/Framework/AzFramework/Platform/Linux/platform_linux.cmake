#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Based on the linux window manager trait, perform the appropriate additional build configurations
# Only 'xcb' and 'wayland' are recognized
if (${PAL_TRAIT_LINUX_WINDOW_MANAGER} STREQUAL "xcb")

    find_library(XCB_LIBRARY xcb)
    find_library(XCB_XKB_LIBRARY xcb-xkb)
    find_library(XKBCOMMON_LIBRARY xkbcommon)
    find_library(XKBCOMMON_X11_LIBRARY xkbcommon-x11)

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            ${XCB_LIBRARY}
            ${XKBCOMMON_LIBRARY}
            ${XKBCOMMON_X11_LIBRARY}
            ${XCB_XKB_LIBRARY}
    )

    set(LY_COMPILE_DEFINITIONS PUBLIC PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB)

elseif(PAL_TRAIT_LINUX_WINDOW_MANAGER STREQUAL "wayland")

    set(LY_COMPILE_DEFINITIONS PUBLIC PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND)

else()

    message(FATAL_ERROR, "Linux Window Manager ${PAL_TRAIT_LINUX_WINDOW_MANAGER} is not recognized")

endif()
