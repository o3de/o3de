#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


if(${CMAKE_ARGC} LESS 9)
    message(FATAL_ERROR "DetachDebugSymbols script called with the wrong number of arguments: ${CMAKE_ARGC}")
endif()

# cmake command arguments (not including the first 2 arguments of '-P', 'DetachDebugSymbols.cmake')

set(GNU_STRIP_TOOL ${CMAKE_ARGV3})          # GNU_STRIP_TOOL      : The location of the gnu strip tool (e.g. /usr/bin/strip)
set(GNU_OBJCOPY ${CMAKE_ARGV4})             # GNU_OBJCOPY         : The location of the gnu objcopy tool (e.g. /usr/bin/objcopy)
set(STRIP_TARGET ${CMAKE_ARGV5})            # STRIP_TARGET        : The built binary to process
set(DEBUG_SYMBOL_EXT ${CMAKE_ARGV6})        # DEBUG_SYMBOL_EXT    : When extracting debug information, the extension to use for the debug database file
set(TARGET_TYPE ${CMAKE_ARGV7})             # TARGET_TYPE         : The target type (STATIC_LIBRARY, MODULE_LIBRARY, SHARED_LIBRARY, EXECUTABLE, or APPLICATION)
set(DEBUG_SYMBOL_OPTION ${CMAKE_ARGV8})     # DEBUG_SYMBOL_OPTION : Either 
                                            #                          - 'DISCARD' : strip debug information except from debug builds
                                            #                          - 'DETACH'  : detach debug information

# This script only supports executables, applications, modules, static libraries, and shared libraries
if (NOT ${TARGET_TYPE} STREQUAL "STATIC_LIBRARY" AND 
    NOT ${TARGET_TYPE} STREQUAL "MODULE_LIBRARY" AND 
    NOT ${TARGET_TYPE} STREQUAL "SHARED_LIBRARY" AND 
    NOT ${TARGET_TYPE} STREQUAL "EXECUTABLE" AND 
    NOT ${TARGET_TYPE} STREQUAL "APPLICATION")
    return()
endif()

# When specifying the location of the debug info file, just use the filename so that the debugger will search
# for the name through the set of known locations. Using the absolute filename will cause the debugger to look
# only for that absolute path, which is not portable
get_filename_component(file_path ${STRIP_TARGET} DIRECTORY)
get_filename_component(filename_only ${STRIP_TARGET} NAME)


if (${DEBUG_SYMBOL_OPTION} STREQUAL "DISCARD")

    message(VERBOSE "Stripping debug symbols from ${STRIP_TARGET}")

elseif (${DEBUG_SYMBOL_OPTION} STREQUAL "DETACH")

    if (${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
        # We will not detach the debug information from static libraries since we are unable to attach them back
        # to that static library. Instead, static libraries will retain their debug information.
        return ()
    endif()

    message(VERBOSE "Detaching debug symbols from ${STRIP_TARGET} into ${filename_only}.${DEBUG_SYMBOL_EXT}")

endif()


# Step 1: Extract the debug information into the debug datafile (For 'DETACH' actions)
if (${DEBUG_SYMBOL_OPTION} STREQUAL "DETACH")

    execute_process(COMMAND 
                        ${GNU_OBJCOPY} --only-keep-debug ${STRIP_TARGET} ${STRIP_TARGET}.${DEBUG_SYMBOL_EXT}
    )
endif()

# Step 2: Stripping out the debug symbols (For both 'DISCARD' and 'DETACH')
execute_process(COMMAND 
                    ${GNU_STRIP_TOOL} --strip-debug ${STRIP_TARGET}
)

# Step 3: Re-link the debug information file from stemp 1 (For 'DETACH' actions)
if (${DEBUG_SYMBOL_OPTION} STREQUAL "DETACH")

    execute_process(COMMAND ${GNU_OBJCOPY} --remove-section=.gnu_debuglink ${STRIP_TARGET})

    execute_process(COMMAND 
                        ${GNU_OBJCOPY} --add-gnu-debuglink=${filename_only}.${DEBUG_SYMBOL_EXT} ${STRIP_TARGET}
                    WORKING_DIRECTORY 
                        ${file_path}
    )
endif()
