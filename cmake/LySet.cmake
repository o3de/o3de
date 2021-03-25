#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

get_directory_property(LY_PARENT_SCOPE PARENT_DIRECTORY)
if(LY_PARENT_SCOPE)
    set(LY_PARENT_SCOPE ${LY_PARENT_SCOPE} PARENT_SCOPE)
endif()

#! ly_set: convenient function to set and export variable to the parent scope in scenarios
#          where CMAKE_SOURCE_DIR != CMAKE_CURRENT_LIST_DIR (e.g. when the engine's cmake 
#          files are included from a project)
#
macro(ly_set)
    set(${ARGN})
    if(LY_PARENT_SCOPE)
        set(${ARGN} PARENT_SCOPE)
    endif()
endmacro()
