#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

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

set(LY_STRIP_DEBUG_SYMBOLS FALSE CACHE BOOL "Flag to strip debug symbols from the output executables and binaries")

#! ly_apply_debug_strip_options: Apply debug stripping options to the target output for non-debug configurations.
#
#\arg:target Name of the target to perform a post-build stripping of any debug symbol)
function(ly_apply_debug_strip_options target)

    if (NOT  ${LY_STRIP_DEBUG_SYMBOLS})
        return()
    endif()

    find_program(LLVM_STRIP_TOOL llvm-strip)
    if (NOT LLVM_STRIP_TOOL)
        message(FATAL_ERROR "Unable to locate 'llvm-strip' tool needed to strip debug symbols from the output target(s). Make sure this is installed on your host machine")
        return()
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "${LY_ROOT_FOLDER}/cmake/Platform/Linux/StripDebugSymbols.cmake"
                "$<TARGET_FILE:${target}>"
                "$<CONFIG>"
        COMMENT "Stripping debug symbols ..."
        VERBATIM
    )

endfunction()
