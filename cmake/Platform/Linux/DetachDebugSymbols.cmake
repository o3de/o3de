#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(NOT ${CMAKE_ARGC} EQUAL 6)
    message(FATAL_ERROR "DetachDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

string(TOLOWER ${CMAKE_ARGV4} STATIC_LIBRARY_SUFFIX)
string(TOLOWER ${CMAKE_ARGV5} DEBUG_SYMBOL_EXT)


find_program(GNU_STRIP_TOOL strip)
find_program(GNU_OBJCOPY objcopy)

get_filename_component(file_extension ${CMAKE_ARGV3} LAST_EXT)

if (GNU_STRIP_TOOL AND GNU_OBJCOPY)

    if (NOT ${file_extension} STREQUAL ${STATIC_LIBRARY_SUFFIX})

        message(STATUS "Detaching debug symbols from ${CMAKE_ARGV3} into ${CMAKE_ARGV3}.${DEBUG_SYMBOL_EXT}")

        execute_process(COMMAND
            ${GNU_OBJCOPY} --only-keep-debug ${CMAKE_ARGV3} ${CMAKE_ARGV3}.${DEBUG_SYMBOL_EXT}
        )

        execute_process(COMMAND
            ${GNU_STRIP_TOOL} --strip-debug ${CMAKE_ARGV3}
        )

        execute_process(COMMAND
            ${GNU_OBJCOPY} --add-gnu-debuglink=${CMAKE_ARGV3}.dbg ${CMAKE_ARGV3}
        )
    endif()

endif()
