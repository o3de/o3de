#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(NOT ${CMAKE_ARGC} EQUAL 6)
    message(FATAL_ERROR "StripDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

set(STRIP_TARGET ${CMAKE_ARGV4})
set(GNU_STRIP_TOOL ${CMAKE_ARGV3})
string(TOLOWER ${CMAKE_ARGV5} CONFIG_LOWER)

if (${CONFIG_LOWER} STREQUAL "debug")

    # Skip the debug-symbol stripping for debug builds

elseif (GNU_STRIP_TOOL)

    message(STATUS "Stripping debug symbols from ${STRIP_TARGET}")
    execute_process(COMMAND
        ${GNU_STRIP_TOOL} --strip-debug ${STRIP_TARGET}
    )

endif()
