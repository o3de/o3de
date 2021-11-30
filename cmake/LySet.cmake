#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_directory_property(LY_PARENT_SCOPE PARENT_DIRECTORY)
if(LY_PARENT_SCOPE)
    set(LY_PARENT_SCOPE ${LY_PARENT_SCOPE} PARENT_SCOPE)
endif()

#! ly_set: convenient function to set and export variable to the parent scope in scenarios
#          where CMAKE_SOURCE_DIR != CMAKE_CURRENT_LIST_DIR (e.g. when the engine's cmake 
#          files are included from a project)
#
macro(ly_set name)
    set(${name} "${ARGN}")
    if(LY_PARENT_SCOPE)
        set(${name} "${ARGN}" PARENT_SCOPE)
    endif()
endmacro()
