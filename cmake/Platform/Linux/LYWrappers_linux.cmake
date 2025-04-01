#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_STRIP_DEBUG_SYMBOLS FALSE CACHE BOOL "Flag to strip debug symbols from the (non-debug) output binaries")
set(LY_DEBUG_SYMBOLS_FILE_EXTENSION "dbg" CACHE STRING "Extension for generated debug symbol files")

# in script only mode, we have no compiler or compiler tools and are always in a situation
# where a project is being built versus a pre-built version of the engine.  This means that
# stripping and copying should not be attempted.
if (NOT O3DE_SCRIPT_ONLY)
    # Check if 'strip' is available so that debug symbols can be stripped from output libraries and executables.
    find_program(GNU_STRIP_TOOL strip)
    if (NOT GNU_STRIP_TOOL)
        message(WARNING "Unable to locate 'strip' tool needed to strip debug symbols from the output target(s). "
                        "Debug symbols will not be removed from output libraries and executables.")
    endif()

    # Check if 'objcopy' is available so that debug symbols can be extracted from output libraries and executables.
    find_program(GNU_OBJCOPY objcopy)
    if (NOT GNU_OBJCOPY)
        message(WARNING "Unable to locate 'objcopy' tool needed to extract debug symbols from the output target(s). "
                        "Debug symbols will not be removed from output libraries and executables. Make sure that "
                        "'objcopy' is installed.")
    endif()
endif()


function(ly_apply_platform_properties target)
    # Noop
endfunction()

function(ly_handle_custom_output_directory target output_subdirectory)

    if(output_subdirectory)
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${output_subdirectory}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${output_subdirectory}
        )

        foreach(conf ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${conf} UCONF)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}/${output_subdirectory}
                LIBRARY_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UCONF}}/${output_subdirectory}
            )
        endforeach()

        # Things like rc plugins are built into a subdirectory, but their
        # dependent libraries end up in the top-level build dir. Their rpath
        # needs to contain $ORIGIN in case they depend on other plugins, and
        # $ORIGIN/.. for the main dependencies.
        if(NOT IS_ABSOLUTE output_subdirectory)
            file(RELATIVE_PATH relative_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${output_subdirectory}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
            set_target_properties(${target} PROPERTIES
                INSTALL_RPATH "$ORIGIN:$ORIGIN/${relative_path}"
            )
        endif()
    endif()

endfunction()

#! ly_apply_debug_strip_options: Apply debug stripping options to the target output for non-debug configurations.
#
#\arg:target Name of the target to perform a post-build stripping of any debug symbol)
function(ly_apply_debug_strip_options target)

    if (NOT GNU_STRIP_TOOL OR NOT GNU_OBJCOPY)
        return()
    endif()

    # If the target is IMPORTED, then there is no post-build process
    get_target_property(is_imported ${target} IMPORTED)
    if (${is_imported})
        return()
    endif()

    # Check the target type
    get_target_property(target_type ${target} TYPE)

    # This script only supports executables, applications, modules, static libraries, and shared libraries
    if (NOT ${target_type} STREQUAL "STATIC_LIBRARY" AND 
        NOT ${target_type} STREQUAL "MODULE_LIBRARY" AND 
        NOT ${target_type} STREQUAL "SHARED_LIBRARY" AND 
        NOT ${target_type} STREQUAL "EXECUTABLE" AND 
        NOT ${target_type} STREQUAL "APPLICATION")
        return()
    endif()

    if (${LY_STRIP_DEBUG_SYMBOLS})
        set(DETACH_DEBUG_OPTION "DISCARD")
    else()
        set(DETACH_DEBUG_OPTION "DETACH")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "${LY_ROOT_FOLDER}/cmake/Platform/Linux/ProcessDebugSymbols.cmake"
                ${GNU_STRIP_TOOL}
                ${GNU_OBJCOPY}
                "$<TARGET_FILE:${target}>"
                ${LY_DEBUG_SYMBOLS_FILE_EXTENSION}
                ${target_type}
                ${DETACH_DEBUG_OPTION}
        COMMENT "Processing debug symbols ..."
        VERBATIM
    )

endfunction()
