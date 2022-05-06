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

# Make sure that both gnu tools 'strip' and 'objcopy' are available
find_program(GNU_STRIP_TOOL strip)
find_program(GNU_OBJCOPY objcopy)

if (GNU_STRIP_TOOL AND GNU_OBJCOPY)

    # Only perform the debug symbol detachment on non-static libraries. (You cannot add the gnu debug link on an static library)
    get_filename_component(file_extension ${CMAKE_ARGV3} LAST_EXT)
    if (NOT ${file_extension} STREQUAL ${STATIC_LIBRARY_SUFFIX})

        # When specifying the location of the debug info file, just use the filename so that the debugger will search
        # for the name through the set of known locations. Using the absolute filename will cause the debugger to look
        # only for that absolute path, which is not portable
        get_filename_component(file_path ${CMAKE_ARGV3} DIRECTORY)
        get_filename_component(filename_only ${CMAKE_ARGV3} NAME)

        message(STATUS "Detaching debug symbols from ${CMAKE_ARGV3} into ${filename_only}.${DEBUG_SYMBOL_EXT}")

        # First extract out the debug symbols into a separate file 
        execute_process(COMMAND 
                            ${GNU_OBJCOPY} --only-keep-debug ${CMAKE_ARGV3} ${CMAKE_ARGV3}.${DEBUG_SYMBOL_EXT}
        )
        # Then strip out the debug symbols completely from the shared library or executable
        execute_process(COMMAND 
                            ${GNU_STRIP_TOOL} --strip-debug ${CMAKE_ARGV3}
        )
        # Then re-link the debug information by creating a .gnu_debuglink section to the filename
        execute_process(COMMAND 
                            ${GNU_OBJCOPY} --add-gnu-debuglink=${filename_only}.${DEBUG_SYMBOL_EXT} ${CMAKE_ARGV3}
                        WORKING_DIRECTORY 
                            file_path
        )
    endif()

endif()
