#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Set compiler options specific to this compiler
function(PAL_Assimp_Lift_CxxFlagsWarnings)
    string(REPLACE "/we5032" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") # detected #pragma warning(push) with no corresponding #pragma warning(pop)
    string(REPLACE "/we4777" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") # 'function' : format string 'string' requires an argument of type 'type1', but variadic argument number has type 'type2
    string(REPLACE "/we4437" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") # dynamic_cast from virtual base 'class1' to 'class2' could fail in some contexts
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

set(O3DE_COMPILE_OPTION_PLATFORM_ASSIMP /wd4777 /wd5032)
