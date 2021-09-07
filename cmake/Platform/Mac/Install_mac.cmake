#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This is used to generate a setreg file which will be placed inside the bundle
# for targets that request it(eg. AssetProcessor/Editor). This is the relative path
# to the bundle from the installed engine's root. This will be used to compute the
# absolute path to bundle which may contain dependent dylibs(eg. Gems) used by a project.
set(installed_binaries_path_template [[
{
    "Amazon": {
        "AzCore": {
            "Runtime": {
                "FilePaths": {
                    "InstalledBinariesFolder": "bin/Mac/$<CONFIG>"
                }
            }
        }
    }
}]]
)

unset(target_conf_dir)
foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${conf} UCONF)
    string(APPEND target_conf_dir $<$<CONFIG:${conf}>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UCONF}}>)
endforeach()

set(installed_binaries_setreg_path ${target_conf_dir}/Registry/installed_binaries_path.setreg)

file(GENERATE OUTPUT ${installed_binaries_setreg_path} CONTENT ${installed_binaries_path_template})

#! ly_install_target_override: Mac specific target installation
function(ly_install_target_override)

    set(options)
    set(oneValueArgs TARGET ARCHIVE_DIR LIBRARY_DIR RUNTIME_DIR LIBRARY_SUBDIR RUNTIME_SUBDIR)
    set(multiValueArgs)
    cmake_parse_arguments(ly_platform_install_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # For bundles on Mac, we set the icons by passing in a path to the Images.xcassets directory.
    # However, the CMake install command expects paths to files for the the RESOURCE property.
    # More details can be found in the CMake issue: https://gitlab.kitware.com/cmake/cmake/-/issues/22409
    get_target_property(is_bundle ${ly_platform_install_target_TARGET} MACOSX_BUNDLE)
    if (${is_bundle})
        get_target_property(cached_resources_dir ${ly_platform_install_target_TARGET} RESOURCE)
        set_property(TARGET ${ly_platform_install_target_TARGET} PROPERTY RESOURCE "")
    endif()
    
    install(
        TARGETS ${ly_platform_install_target_TARGET}
        ARCHIVE
            DESTINATION ${ly_platform_install_target_ARCHIVE_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        LIBRARY
            DESTINATION ${ly_platform_install_target_LIBRARY_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_LIBRARY_SUBDIR}
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        RUNTIME
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        BUNDLE
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
        RESOURCE
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}/
            COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

    if (${is_bundle})
        set_property(TARGET ${ly_platform_install_target_TARGET} PROPERTY RESOURCE ${cached_resources_dir})
    endif()
endfunction()

#! ly_install_add_install_path_setreg: Adds the install path setreg file as a dependency
function(ly_install_add_install_path_setreg NAME)
    set_property(TARGET ${NAME} APPEND PROPERTY INTERFACE_LY_TARGET_FILES "${installed_binaries_setreg_path}\nRegistry")
endfunction()

#! ly_install_code_function_override: Mac specific copy function to handle frameworks
function(ly_install_code_function_override)

    install(CODE
"function(ly_copy source_file target_directory)
    if(\"\${source_file}\" MATCHES \"\\\\.[Ff]ramework[^\\\\.]\")

        # fixup origin to copy the whole Framework folder
        string(REGEX REPLACE \"(.*\\\\.[Ff]ramework).*\" \"\\\\1\" source_file \"\${source_file}\")
        get_filename_component(target_filename \"\${source_file}\" NAME)

    endif()
    file(COPY \"\${source_file}\" DESTINATION \"\${target_directory}\" FILE_PERMISSIONS ${LY_COPY_PERMISSIONS})
endfunction()"
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
    )

endfunction()

include(cmake/Platform/Common/Install_common.cmake)
