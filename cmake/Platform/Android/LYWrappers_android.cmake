#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_STRIP_DEBUG_SYMBOLS FALSE CACHE BOOL "Flag to strip debug symbols from the (non-debug) output binaries")

# Check if 'llvm-strip' is available so that debug symbols can be stripped from output libraries and executables.
find_program(LLVM_STRIP_TOOL llvm-strip)
if (NOT LLVM_STRIP_TOOL)
    message(WARNING "Unable to locate 'strip' tool needed to strip debug symbols from the output target(s). "
                    "Debug symbols will not be removed from output libraries and executables.")
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

    endif()

endfunction()

#! ly_apply_debug_strip_options: Apply debug stripping options to the target output for non-debug configurations.
#
#\arg:target Name of the target to perform a post-build stripping of any debug symbol)
function(ly_apply_debug_strip_options target)

    if (NOT LLVM_STRIP_TOOL)
        return()
    endif()

    if (NOT LY_STRIP_DEBUG_SYMBOLS)
        return()
    endif()

    # If the target is IMPORTED, then there is no post-build process
    get_target_property(is_imported ${target} IMPORTED)
    if (${is_imported})
        return()
    endif()

    # Check the target type
    get_target_property(target_type ${target} TYPE)

    # This script only supports executables, applications, modules, and shared libraries
    if (NOT ${target_type} STREQUAL "MODULE_LIBRARY" AND
        NOT ${target_type} STREQUAL "SHARED_LIBRARY" AND
        NOT ${target_type} STREQUAL "EXECUTABLE" AND
        NOT ${target_type} STREQUAL "APPLICATION")
        return()
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "${LY_ROOT_FOLDER}/cmake/Platform/Android/StripDebugSymbols.cmake"
                ${LLVM_STRIP_TOOL}
                "$<TARGET_FILE:${target}>"
                ${target_type}
        COMMENT "Stripping debug symbols ..."
        VERBATIM
    )

endfunction()
