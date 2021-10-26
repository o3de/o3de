#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_ENABLE_HARDENED_RUNTIME OFF CACHE BOOL "Enable hardened runtime capability for Mac builds. This should be ON when building the engine for notarization/distribution.")

define_property(TARGET PROPERTY ENTITLEMENT_FILE_PATH
    BRIEF_DOCS "Path to the entitlement file"
    FULL_DOCS [[
        On MacOS, entitlements are used to grant certain privileges
        to applications at runtime. Use this propery to specify the
        path to a .plist file containing entitlements.
    ]]
)

function(ly_apply_platform_properties target)

    set_target_properties(${target} PROPERTIES
        BUILD_RPATH "@executable_path/;@executable_path/../Frameworks"
        INSTALL_RPATH "@executable_path/;@executable_path/../Frameworks"
    )

    get_property(is_imported TARGET ${target} PROPERTY IMPORTED)
    if((NOT is_imported) AND (LY_ENABLE_HARDENED_RUNTIME))
        get_property(target_type TARGET ${target} PROPERTY TYPE)
        set(runtime_types_list "MODULE_LIBRARY" "SHARED_LIBRARY" "EXECUTABLE")
        if (target_type IN_LIST runtime_types_list)
            set_target_properties(${target} PROPERTIES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
                XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS NO
            )
        endif()
    endif()

endfunction()


function(ly_handle_custom_output_directory target output_subdirectory)

    if(output_subdirectory)
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${output_subdirectory}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${output_subdirectory}
        )

        foreach(conf ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${conf} UCONF)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}/${output_subdirectory}
                LIBRARY_OUTPUT_DIRECTORY_${UCONF} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}/${output_subdirectory}
            )
        endforeach()

    endif()

endfunction()

#! ly_add_bundle_resources: add resource files to the current bundle application (if any) in the bundle's resource folder
#  if a bundle is specified for the supporting platform.
#
# \arg:FILES files to copy to the resources
#
function(ly_add_bundle_resources)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs FILES)

    cmake_parse_arguments(ly_add_bundle_resources "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_add_bundle_resources_TARGET)
        message(FATAL_ERROR "You must provide a TARGET")
    endif()

    if(NOT ly_add_bundle_resources_FILES)
        message(FATAL_ERROR "You must provide at least a file to copy")
    endif()

    if (TARGET ${ly_add_bundle_resources_TARGET})

        set(destination_location $<TARGET_BUNDLE_CONTENT_DIR:${ly_add_bundle_resources_TARGET}>/Resources)

        foreach(file ${ly_add_bundle_resources_FILES})

            get_filename_component(filename ${file} NAME)
            add_custom_command(
                TARGET ${ly_add_bundle_resources_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${destination_location}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file} ${destination_location}
                DEPENDS ${file}
                VERBATIM
                COMMENT "Copying ${file} to Resources..."
            )

        endforeach()

    endif()

endfunction()
