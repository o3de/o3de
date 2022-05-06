#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(NOT ${CMAKE_ARGC} EQUAL 5)
    message(FATAL_ERROR "StripDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

string(TOLOWER ${CMAKE_ARGV4} CONFIG_LOWER)

find_program(GNU_STRIP_TOOL strip)

if (${CONFIG_LOWER} STREQUAL "debug")

    # Skip the debug-symbol stripping for debug builds

elseif (GNU_STRIP_TOOL)

    message(STATUS "Stripping debug symbols from ${CMAKE_ARGV3}")
    execute_process(COMMAND
        ${GNU_STRIP_TOOL} --strip-debug ${CMAKE_ARGV3}
    )

else()

    message(STATUS "Unable to strip debug symbols for ${CMAKE_ARGV3}: Unable to locate 'strip' command")

endif()
