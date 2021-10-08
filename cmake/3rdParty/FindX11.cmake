#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Open a new scope so we can make changes to CMAKE_MODULE_PATH, and restore it
# when we're done
function(FindX11)
    # O3DE's FindX11.cmake is a wrapper for the one that CMake provides. Remove
    # our current directory from CMAKE_MODULE_PATH to avoid recursive includes
    list(REMOVE_ITEM CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

    find_package(X11 COMPONENTS ${X11_FIND_COMPONENTS} QUIET)

    foreach(component IN LISTS X11_FIND_COMPONENTS)
        ly_add_external_target(
            SYSTEM
            PACKAGE X11
            NAME ${component}
            VERSION ""
            BUILD_DEPENDENCIES
                X11::${component}
        )
    endforeach()
endfunction()
FindX11()
