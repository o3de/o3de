#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(NOT ${CMAKE_ARGC} EQUAL 7)
    message(FATAL_ERROR "DetachDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

set(GNU_STRIP_TOOL ${CMAKE_ARGV3})
set(GNU_OBJCOPY ${CMAKE_ARGV4})
set(STRIP_TARGET ${CMAKE_ARGV5})
set(DEBUG_SYMBOL_EXT ${CMAKE_ARGV6})

# When specifying the location of the debug info file, just use the filename so that the debugger will search
# for the name through the set of known locations. Using the absolute filename will cause the debugger to look
# only for that absolute path, which is not portable
get_filename_component(file_path ${STRIP_TARGET} DIRECTORY)
get_filename_component(filename_only ${STRIP_TARGET} NAME)

message(STATUS "Detaching debug symbols from ${STRIP_TARGET} into ${filename_only}.${DEBUG_SYMBOL_EXT}")

# First extract out the debug symbols into a separate file 
execute_process(COMMAND 
                    ${GNU_OBJCOPY} --only-keep-debug ${STRIP_TARGET} ${STRIP_TARGET}.${DEBUG_SYMBOL_EXT}
)
# Then strip out the debug symbols completely from the shared library or executable
execute_process(COMMAND 
                    ${GNU_STRIP_TOOL} --strip-debug ${STRIP_TARGET}
)
# Then re-link the debug information by creating a .gnu_debuglink section to the filename
execute_process(COMMAND 
                    ${GNU_OBJCOPY} --add-gnu-debuglink=${filename_only}.${DEBUG_SYMBOL_EXT} ${STRIP_TARGET}
                WORKING_DIRECTORY 
                    ${file_path}
)
