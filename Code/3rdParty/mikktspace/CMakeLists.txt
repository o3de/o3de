#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

include(CheckCSourceCompiles)

set(mikktspace_math_test [[
    #include <math.h>

    int main(void)
    {
        volatile double value = 0.5;
        return (int)(acos(value) + cos(value));
    }
]])

check_c_source_compiles("${mikktspace_math_test}" MIKKTSPACE_MATH_IN_C_RUNTIME)

set(mikktspace_system_libraries)
if(NOT MIKKTSPACE_MATH_IN_C_RUNTIME)
    block()
        set(CMAKE_REQUIRED_LIBRARIES m)
        check_c_source_compiles("${mikktspace_math_test}" MIKKTSPACE_MATH_IN_LIBM)
    endblock()

    if(NOT MIKKTSPACE_MATH_IN_LIBM)
        message(FATAL_ERROR "MikkTSpace requires acos() and cos(), but they could not be linked from the C runtime or libm")
    endif()

    list(APPEND mikktspace_system_libraries PUBLIC m)
endif()

ly_add_3rdparty_target(
    NAME mikktspace STATIC
    INCLUDE_DIRECTORIES
        PUBLIC Include
        PRIVATE Include/MikkTSpace
    BUILD_DEPENDENCIES
        ${mikktspace_system_libraries}
)

ly_create_alias(NAME MikkTSpace NAMESPACE 3rdParty TARGETS 3rdParty::mikktspace)
ly_create_alias(NAME mikkelsen NAMESPACE 3rdParty TARGETS 3rdParty::mikktspace)
