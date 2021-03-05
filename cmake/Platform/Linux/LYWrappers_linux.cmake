#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
